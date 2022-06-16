#include "registry.h"
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
#include "imgui/IconsFontAwesome5.h"

using namespace xs;
using namespace xs::tools;
using namespace std;

namespace xs::registry::internal
{
	struct registry_value
	{
		type type = type::none;
		std::variant<double, bool, uint32_t, std::string> value;
	};

	std::unordered_map<std::string, registry_value> reg;

	template<typename T>
	T get(const std::string& name);

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
}

template<class T>
T xs::registry::internal::get<T>(const std::string& name)
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
			xs::log::warn("Registry value with name '{}' is of different type.", name);
		}
	}
	else
	{
		xs::log::warn("Registry value with name '{}' not found. Adding default to registry.", name);
		T t = {};
		internal::reg[name] = { type::game, t };
	}
	
	return {};
}

template<typename T>
void xs::registry::internal::set(const std::string& name, const T& value, type type)
{
	internal::reg[name] = { type, value };
}

using namespace xs::registry::internal;

void xs::registry::initialize()
{
	load_of_type(type::game);
	load_of_type(type::system);
	load_of_type(type::player);
}

void xs::registry::shutdown() {}

void xs::registry::inspect(bool& show)
{
	ImGui::Begin(u8"\U0000f1c0  Data", &show, ImGuiWindowFlags_NoCollapse);

	if (ImGui::Button(ICON_FA_UNDO))
	{
	}
	tooltip("Undo not implemented");
	ImGui::SameLine();

	if (ImGui::Button(ICON_FA_REDO))
	{
	}
	tooltip("Redo not implemented");
	ImGui::SameLine();

	static ImGuiTextFilter filter;
	filter.Draw(ICON_FA_SEARCH);
	
	inspect_of_type(string(ICON_FA_GAMEPAD) + "  Game", filter, type::game);
	inspect_of_type(string(ICON_FA_USER) + "  Player", filter, type::player);
	inspect_of_type(string(ICON_FA_BUG) + "  Debug", filter, type::debug);
	inspect_of_type(string(ICON_FA_COG) + "  System", filter, type::system);

	ImGui::End();
}

double xs::registry::get_number(const std::string& name)
{
	return get<double>(name);
}

uint32_t xs::registry::get_color(const std::string& name)
{
	return get<uint32_t>(name);
}

bool xs::registry::get_bool(const std::string& name)
{
	return get<bool>(name);
}

std::string xs::registry::get_string(const std::string& name)
{
	return "";
}

void xs::registry::set_number(const std::string& name, double value, type tp)
{
	set(name, value, tp);
}

void xs::registry::set_color(const std::string& name, uint32_t value, type tp)
{
	set(name, value, tp);
}

void xs::registry::set_bool(const std::string& name, bool value, type tp)
{
	set(name, value, tp);
}

void xs::registry::set_string(const std::string& name, const std::string& value, type tp)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////									Internal Impl
////////////////////////////////////////////////////////////////////////////////////////////////////
void xs::registry::internal::inspect_of_type(
	const std::string& type_name,
	ImGuiTextFilter& filter,
	xs::registry::type type)
{
	if (ImGui::CollapsingHeader(type_name.c_str()))
	{
		for (auto& itr : reg)
			if (filter.PassFilter(itr.first.c_str()) && itr.second.type == type)
				inspect_entry(itr);

		if (type == type::system || type == type::game || type == type::player)
		{
			if (ImGui::Button("Save"))
				save_of_type(type);
			tooltip("Save to a file");
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::Text(fileio::get_path(get_file_path(type)).c_str());
			ImGui::PopStyleColor();
		}
	}	
}

void xs::registry::internal::save_of_type(type type)
{
	nlohmann::json j;
	for (auto& itr : reg)
	{
		if (itr.second.type == type)
		{
			auto val_double = std::get_if<double>(&itr.second.value);
			auto val_bool = std::get_if<bool>(&itr.second.value);
			auto val_uint32_t = std::get_if<uint32_t>(&itr.second.value);

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

void xs::registry::internal::load_of_type(type type)
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
		}
	}
}

const string& xs::registry::internal::get_file_path(type type)
{
	static std::string game_path = "[cwd]/game.json";
	static std::string player_path = "[save]/player.json";
	static std::string system_path = "[cwd]/system.json";
	static std::string no_path = "";

	switch (type)
	{
	case xs::registry::type::none:
		return no_path;
	case xs::registry::type::system:
		return system_path;
	case xs::registry::type::debug:
		return no_path;
	case xs::registry::type::game:
		return game_path;
	case xs::registry::type::player:
		return player_path;	
	}

	return no_path;
}

void xs::registry::internal::inspect_entry(
	std::pair<const std::string,
	xs::registry::internal::registry_value>& itr)
{
	// TODO: Replace with get_if
	try
	{
		auto val = std::get<double>(itr.second.value);
		float flt = (float)val;
		ImGui::DragFloat(itr.first.c_str(), &flt, 0.01f);
		set(itr.first, flt, itr.second.type);
		return;
	}
	catch (...) {}

	try
	{
		auto val = std::get<bool>(itr.second.value);
		ImGui::Checkbox(itr.first.c_str(), &val);
		set(itr.first, val, itr.second.type);
		return;
	}
	catch (...) {}

	try
	{
		auto val = std::get<uint32_t>(itr.second.value);

		ImVec4 vec = color_convert(val);
		ImGui::ColorEdit4(itr.first.c_str(), &vec.x);
		val = color_convert(vec);
		set(itr.first, val, itr.second.type);
		return;
	}
	catch (...) {}
}

void xs::registry::internal::tooltip(const char* tooltip)
{
	if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.6f)
	{
		ImGui::BeginTooltip();
		ImGui::SetTooltip(tooltip);
		ImGui::EndTooltip();
	}
}
