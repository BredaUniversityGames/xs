#include "registry.h"
#include <unordered_map>
#include <memory>
#include <any>
#include <variant>
#include "log.h"
#include "render.h"
#include "tools.h"
#include "imgui/imgui.h"

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

	/*
	ImVec4 ImGui::ColorConvertU32ToFloat4(ImU32 in)
	{
		float s = 1.0f / 255.0f;
		return ImVec4(
			((in >> IM_COL32_R_SHIFT) & 0xFF) * s,
			((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
			((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
			((in >> IM_COL32_A_SHIFT) & 0xFF) * s);
	}

	ImU32 ImGui::ColorConvertFloat4ToU32(const ImVec4& in)
	{
		ImU32 out;
		out = ((ImU32)IM_F32_TO_INT8_SAT(in.x)) << IM_COL32_R_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(in.y)) << IM_COL32_G_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(in.z)) << IM_COL32_B_SHIFT;
		out |= ((ImU32)IM_F32_TO_INT8_SAT(in.w)) << IM_COL32_A_SHIFT;
		return out;
	}
	*/

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
	ImGui::Begin("Registry");

	for (auto& itr : reg)
	{
		try
		{
			auto val = std::get<double>(itr.second.value);
			float flt = (float)val;
			ImGui::DragFloat(itr.first.c_str(), &flt, 0.01f);
			set(itr.first, flt, itr.second.type);
			continue;
		}
		catch(...) {}

		try
		{
			auto val = std::get<bool>(itr.second.value);
			ImGui::Checkbox(itr.first.c_str(), &val);
			set(itr.first, val, itr.second.type);
			continue;
		}
		catch (...) {}

		try
		{
			auto val = std::get<uint32_t>(itr.second.value);
			
			ImVec4 vec = color_convert(val);
			//ImVec4 vec = ImGui::ColorConvertU32ToFloat4(val);
			ImGui::ColorEdit4(itr.first.c_str(), &vec.x);
			//val = ImGui::ColorConvertFloat4ToU32(vec);
			val = color_convert(vec);
			set(itr.first, val, itr.second.type);
			continue;
		}
		catch (...) {}
	}

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



