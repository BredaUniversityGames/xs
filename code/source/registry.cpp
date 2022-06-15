#include "registry.h"
#include <unordered_map>
#include <memory>
#include <any>
#include <variant>
#include "log.h"
#include "render.h"
#include "tools.h"
#include "imgui/imgui.h"
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
		internal::reg[name] = { type::game, {} };
	}
	
	return {};
}

template<typename T>
void xs::registry::internal::set(const std::string& name, const T& value, type type)
{
	internal::reg[name] = { type, value };
}

using namespace xs::registry::internal;

void xs::registry::initialize() {}

void xs::registry::shutdown() {}

void xs::registry::inspect()
{
	ImGui::Begin(u8"\U0000F013  Registry");

	static ImGuiTextFilter filter;
	filter.Draw(ICON_FA_SEARCH);
	
	inspect_of_type(string(ICON_FA_GAMEPAD) + "  Game", filter, type::game);
	inspect_of_type(string(ICON_FA_USER) + "  Player", filter, type::player);
	inspect_of_type(string(ICON_FA_BUG) + "  Debug", filter, type::debug);
	inspect_of_type(string(ICON_FA_COG) + "  System", filter, type::system);

	ImGui::End();
}

void xs::registry::internal::inspect_of_type(
	const std::string& type_name,
	ImGuiTextFilter& filter,
	xs::registry::type type)
{
	if (ImGui::CollapsingHeader(type_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (auto& itr : reg)
			if (filter.PassFilter(itr.first.c_str()) && itr.second.type == type)
				inspect_entry(itr);
	}
}

void xs::registry::internal::inspect_entry(
	std::pair<const std::string,
	xs::registry::internal::registry_value>& itr)
{
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



