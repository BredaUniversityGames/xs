#include "registry.h"
#include <unordered_map>
#include <memory>
#include "log.h"
#include "imgui/imgui.h"

using namespace xs;
using namespace std;

namespace xs::registry::internal
{
	enum class type
	{
		none,
		system,
		game,
		player
	};

	struct registry_value
	{
		type type = type::none;
	};

	struct registry_number : public registry_value
	{
		registry_number(double v) { value = v; }
		double value = 0.0;
	};

	struct registry_color : public registry_value
	{
		registry_color(uint32_t v) { value = v; }
		uint32_t value = 0;
	};

	struct registry_string : public registry_value
	{
		std::string value = {};
	};


	std::unordered_map<std::string, std::unique_ptr<registry_value>> reg;
}

using namespace xs::registry::internal;

void xs::registry::initialize()
{
}

void xs::registry::shutdown()
{
}

void xs::registry::inspect()
{
	ImGui::Begin("Registry");
	for (auto &itr : reg)
	{
		auto ptr = itr.second.get();
		auto as_num = static_cast<registry_number*>(ptr);
		auto as_col = static_cast<registry_color*>(ptr);
		if (as_num)
		{
			float flt = (float)as_num->value;
			ImGui::DragFloat(itr.first.c_str(), &flt, 0.01f);
			as_num->value = flt;
		}
		else if (as_col)
		{
			//xs::render::color
			auto vec = ImGui::ColorConvertU32ToFloat4(as_col->value);
			ImGui::ColorEdit4(itr.first.c_str(), &vec.x);
			as_col->value = ImGui::ColorConvertFloat4ToU32(vec);
		}
	}
	ImGui::End();

}

double xs::registry::get_number(const std::string& name)
{
	auto itr = internal::reg.find(name);
	if (itr != internal::reg.end())
	{
		auto ptr = itr->second.get();
		auto dptr = static_cast<internal::registry_number*>(ptr);
		if (dptr)
			return dptr->value;
		else 
			xs::log::warn("Registry value with name '{}' is not a number.", name);
	}
	else
	{
		xs::log::warn("Registry value with name '{}' not found.", name);
		internal::reg[name] = make_unique<registry_number>(0.0);
	}
	return 0.0;
}

uint32_t xs::registry::get_color(const std::string& name)
{
	auto itr = internal::reg.find(name);
	if (itr != internal::reg.end())
	{
		auto ptr = itr->second.get();
		auto dptr = static_cast<internal::registry_color*>(ptr);
		if (dptr)
			return dptr->value;
		else
			xs::log::warn("Registry value with name '{}' is not a color.", name);
	}
	else
	{
		xs::log::warn("Registry value with name '{}' not found.", name);
		internal::reg[name] = make_unique<registry_color>(0);
	}
	return 0;
}


