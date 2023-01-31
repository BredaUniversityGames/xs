#include "data.h"
#include <unordered_map>
#include <memory>
#include <any>
#include <variant>
#include <fstream>
#include "log.h"
#include "render.h"
#include "tools.h"
#include "fileio.h"
#include "json/json.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/cpp/imgui_stdlib.h"
#include "imgui/IconsFontAwesome5.h"

using namespace xs;
using namespace xs::tools;
using namespace std;

namespace xs::data::internal
{
	struct registry_value
	{
		type type = type::none;
		std::variant<double, bool, uint32_t, std::string> value;
	};

	using regsitry_type = std::unordered_map<std::string, registry_value>;
	regsitry_type reg;

	vector<regsitry_type> history;
	int history_stack_pointer;

	template<class T>
	T get(const std::string& name, type type);

	template<typename T>
	void set(const std::string& name, const T& reg_value, type type);

	uint32_t color_convert(ImVec4 color)
	{
		ImU32 out;
		out = (f32_to_uint8(color.x)) << 24;
		out |= (f32_to_uint8(color.y)) << 16;
		out |= (f32_to_uint8(color.z)) << 8;
		out |= (f32_to_uint8(color.w)) << 0;
		return out;
	}

	ImVec4 color_convert(uint32_t color)
	{
		float s = 1.0f / 255.0f;
		return ImVec4(
			((color >> 24) & 0xFF) * s,
			((color >> 16) & 0xFF) * s,
			((color >> 8) & 0xFF) * s,
			((color >> 0) & 0xFF) * s);
	}

	void inspect_entry(std::pair<const std::string, registry_value>& itr);
	void inspect_of_type(const std::string& type_name, ImGuiTextFilter& filter, type type);
	void save_of_type(type type);
	void load_of_type(type type);
	const string& get_file_path(type type);

	void tooltip(const char* tooltip);
	void undo();
	void redo();
}

template<class T>
T xs::data::internal::get(const std::string& name, type type)
{
	auto itr = internal::reg.find(name);
	if (itr != internal::reg.end())
	{	
		try
		{
			auto val = std::get<T>(itr->second.value);
			return val;
		}
		catch (...)
		{			
			xs::log::warn("Data value with name '{}' is of different type.", name);
		}
	}
	else
	{
		xs::log::warn("Data value with name '{}' not found. Adding default to data.", name);
		T t = {};
		internal::reg[name] = { type, t };
	}
	
	return {};
}

template<typename T>
void xs::data::internal::set(const std::string& name, const T& value, type type)
{
	internal::reg[name] = { type, value };
}

using namespace xs::data::internal;

void xs::data::initialize()
{
	load_of_type(type::game);
	load_of_type(type::system);
	load_of_type(type::player);
	load_of_type(type::debug);
}

void xs::data::shutdown() {}

void xs::data::inspect(bool& show)
{	
	if (history.empty())
	{
		auto r(reg);
		history.push_back(r);
	}

	ImGui::Begin(u8"\U0000f1c0  Data", &show);	

	ImGui::BeginDisabled(!(internal::history_stack_pointer < history.size() - 1));
	if (ImGui::Button(ICON_FA_UNDO))
	{
		internal::undo();
	}
	tooltip("Undo");
	ImGui::EndDisabled();
	ImGui::SameLine();

	ImGui::BeginDisabled(internal::history_stack_pointer == 0);
	if (ImGui::Button(ICON_FA_REDO))
	{
		internal::redo();
	}	
	tooltip("Redo");
	ImGui::EndDisabled();
	ImGui::SameLine();

	static ImGuiTextFilter filter;
	filter.Draw(ICON_FA_SEARCH);
	
	ImGui::BeginChild("Child");
	ImGui::PushItemWidth(80);
	inspect_of_type(string(ICON_FA_GAMEPAD) + "  Game", filter, type::game);
	inspect_of_type(string(ICON_FA_USER) + "  Player", filter, type::player);
	inspect_of_type(string(ICON_FA_BUG) + "  Debug", filter, type::debug);
	inspect_of_type(string(ICON_FA_COG) + "  System", filter, type::system);
	ImGui::PopItemWidth();
	ImGui::EndChild();

	ImGui::End();
}

double xs::data::get_number(const std::string& name, type type)
{
	return get<double>(name, type);
}

uint32_t xs::data::get_color(const std::string& name, type type)
{
	return get<uint32_t>(name, type);
}

bool xs::data::get_bool(const std::string& name, type type)
{
	return get<bool>(name, type);
}

std::string xs::data::get_string(const std::string& name, type type)
{
	return get<string>(name, type);
}

void xs::data::set_number(const std::string& name, double value, type tp)
{
	set<double>(name, value, tp);
}

void xs::data::set_color(const std::string& name, uint32_t value, type tp)
{
	set<uint32_t>(name, value, tp);
}

void xs::data::set_bool(const std::string& name, bool value, type tp)
{
	set<bool>(name, value, tp);
}

void xs::data::set_string(const std::string& name, const std::string& value, type tp)
{
	set<string>(name, value, tp);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////									Internal Impl
////////////////////////////////////////////////////////////////////////////////////////////////////
void xs::data::internal::inspect_of_type(
	const string& type_name,
	ImGuiTextFilter& filter,
	xs::data::type type)
{
	if (ImGui::CollapsingHeader(type_name.c_str()))
	{
		vector<string> sorted;
		for (auto& itr : reg)
			if (filter.PassFilter(itr.first.c_str()) && itr.second.type == type)
				sorted.push_back(itr.first);
		sort(sorted.begin(), sorted.end());

		for(const auto& s : sorted)
			inspect_entry(*reg.find(s));

		if (ImGui::Button("Save"))
			save_of_type(type);
		tooltip("Save to a file");
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		ImGui::Text("%s", fileio::get_path(get_file_path(type)).c_str());
		ImGui::PopStyleColor();
	}	
}

void xs::data::internal::save_of_type(type type)
{
	nlohmann::json j;
	for (auto& itr : reg)
	{
		if (itr.second.type == type)
		{
			auto val_double = std::get_if<double>(&itr.second.value);
			auto val_bool = std::get_if<bool>(&itr.second.value);
			auto val_uint32_t = std::get_if<uint32_t>(&itr.second.value);
			auto val_string = std::get_if<string>(&itr.second.value);

			if (val_double)
			{
				j[itr.first]["value"] = *val_double;
				j[itr.first]["type"] = "number";
			}
			else if (val_bool)
			{
				j[itr.first]["value"] = *val_bool;
				j[itr.first]["type"] = "bool";
			}
			else if (val_uint32_t)
			{
				j[itr.first]["value"] = *val_uint32_t;
				j[itr.first]["type"] = "color";
			}
			else if (val_string)
			{
				j[itr.first]["value"] = *val_string;
				j[itr.first]["type"] = "string";
			}
		}
	}
	
	auto filename = fileio::get_path(get_file_path(type));
	std::ofstream ofs;
	ofs.open(filename);
	if (ofs.is_open())
	{
		auto str = j.dump(4);
		ofs << str;
		ofs.close();
	}
}

void xs::data::internal::load_of_type(type type)
{	
	auto filename = fileio::get_path(get_file_path(type));
	std::ifstream ifs;
	ifs.open(filename);
	if (ifs.is_open())
	{
		nlohmann::json j = nlohmann::json::parse(ifs);
		for (auto it = j.begin(); it != j.end(); ++it)
		{
			auto name = it.key();
			auto entry = it.value();
			auto value = entry["value"];
			auto etype = entry["type"];
			if (etype == "color")
				set_color(name, (uint32_t)value, type);
			else if(etype == "bool")
				set_bool(name, (bool)value, type);
			else if(etype == "number")
				set_number(name, (double)value, type);
			else if (etype == "string")
				set_string(name, value, type);
		}
	}
}

const string& xs::data::internal::get_file_path(type type)
{
	static std::string game_path = "[game]/game.json";
	static std::string player_path = "[save]/player.json";
	static std::string system_path = "[game]/system.json";
	static std::string debug_path = "[save]/debug.json";
	static std::string no_path = "";

	switch (type)
	{
	case xs::data::type::none:
		return no_path;
	case xs::data::type::system:
		return system_path;
	case xs::data::type::debug:
		return debug_path;
	case xs::data::type::game:
		return game_path;
	case xs::data::type::player:
		return player_path;	
	}

	return no_path;
}

void xs::data::internal::inspect_entry(
	std::pair<const std::string,
	xs::data::internal::registry_value>& itr)
{

	{
		auto val = std::get_if<double>(&itr.second.value);
		if(val)
		{
			auto val = std::get<double>(itr.second.value);
			float flt = (float)val;
			ImGui::DragFloat(itr.first.c_str(), &flt, 0.01f);
			set(itr.first, flt, itr.second.type);
		}
	}

	{

		auto val = std::get_if<bool>(&itr.second.value);
		if (val)
		{
			ImGui::Checkbox(itr.first.c_str(), val);
			set(itr.first, *val, itr.second.type);
		}
	}

	{
		auto val = std::get_if<uint32_t>(&itr.second.value);
		if (val)
		{
			ImVec4 vec = color_convert(*val);
			ImGui::ColorEdit4(itr.first.c_str(), &vec.x);
			*val = color_convert(vec);
			set(itr.first, *val, itr.second.type);
		}
	}

	{
		auto val = std::get_if<string>(&itr.second.value);
		if (val)
		{			
			ImGui::PushItemWidth(0);
			ImGui::InputText(itr.first.c_str(), val);
			ImGui::PopItemWidth();
			set(itr.first, *val, itr.second.type);
		}
	}


	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		if (history_stack_pointer > 0)
		{
			history.resize(history.size() - history_stack_pointer);
			history_stack_pointer = 0;
		}

		regsitry_type r(reg);
		history.push_back(r);
	}
}

void xs::data::internal::tooltip(const char* tooltip)
{
	if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.6f)
	{
		ImGui::BeginTooltip();
		ImGui::SetTooltip("%s", tooltip);
		ImGui::EndTooltip();
	}
}

void xs::data::internal::undo()
{
	if(internal::history_stack_pointer < history.size() - 1)
		internal::history_stack_pointer++;

	size_t idx = history.size() - 1 - history_stack_pointer;
	if (idx < history.size() && idx >= 0) {
		regsitry_type& r = history[idx];
		reg = r;
	}
}

void xs::data::internal::redo()
{
	if (internal::history_stack_pointer > 0)
		internal::history_stack_pointer--;

	size_t idx = history.size() - 1 - history_stack_pointer;
	if (idx < history.size() && idx >= 0) {
		regsitry_type& r = history[idx];
		reg = r;
	}
}
