#include "data.hpp"
#include <unordered_map>
#include <map>
#include <memory>
#include <any>
#include <variant>
#include <fstream>
#include "log.hpp"
#include "render.hpp"
#include "tools.hpp"
#include "fileio.hpp"
#include "inspector.hpp"
#include "json/json.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include "fluent_glyph.hpp"

using namespace xs;
using namespace xs::tools;
using namespace std;

namespace xs::data::internal
{
	struct registry_value
	{
		type data_type = type::none;
		std::variant<double, bool, uint32_t, std::string> value;
		bool active = false;
	};

	using regsitry_type = std::unordered_map<std::string, registry_value>;
	regsitry_type reg;

	vector<regsitry_type> history;
	int history_stack_pointer;

	std::unordered_map<xs::data::type, bool> edited;

	template<class T>
	T get(const std::string& name, type type, const T& default_value = T{});

	template<typename T>
	void set(const std::string& name, const T& reg_value, type type, bool active = false);

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

	void inspect_of_type(const std::string& name, const std::string& icon, type type);

	// Returns the leaf display label: last dot-segment of the base name (before any '|').
	// "Render.Postprocess.Enabled"  -> "Enabled"
	// "Device.Width"                -> "Width"
	// "Volume"                      -> "Volume"
	// "Render.Mode|Opt1|Opt2"       -> "Mode"
	static string get_leaf_name(const string& key)
	{
		auto base_end = key.find('|');
		auto base = (base_end != string::npos) ? key.substr(0, base_end) : key;
		auto dot = base.rfind('.');
		return (dot != string::npos) ? base.substr(dot + 1) : base;
	}

	// Recursive tree node: sub-groups keyed by name, leaf registry keys at this level.
	struct tree_node
	{
		map<string, tree_node> children;
		vector<string>         entries;
	};

	// Build a recursive tree from a sorted list of registry keys.
	// "Render.Postprocess.Enabled" -> root["Render"]["Postprocess"].entries << key
	// "Volume"                     -> root.entries << key
	static tree_node build_tree(const vector<string>& keys)
	{
		tree_node root;
		for (const auto& key : keys)
		{
			auto base_end = key.find('|');
			auto base = (base_end != string::npos) ? key.substr(0, base_end) : key;

			// Split base on '.'
			vector<string> segments;
			size_t start = 0;
			size_t pos;
			while ((pos = base.find('.', start)) != string::npos)
			{
				segments.push_back(base.substr(start, pos - start));
				start = pos + 1;
			}
			// leaf segment (last or only part)
			segments.push_back(base.substr(start));

			// Navigate / create tree nodes for all but the last segment
			tree_node* node = &root;
			for (size_t i = 0; i + 1 < segments.size(); ++i)
				node = &node->children[segments[i]];

			node->entries.push_back(key);
		}
		return root;
	}

	void load_of_type(type type);
	const string& get_file_path(type type);

	void tooltip(const char* tooltip);
	void undo();
	void redo();
}

template<class T>
T xs::data::internal::get(const std::string& name, type type, const T& default_value)
{
	auto itr = internal::reg.find(name);
	if (itr != internal::reg.end())
	{	
		try
		{
			auto val = std::get<T>(itr->second.value);
			itr->second.active = true;
			return val;
		}
		catch (...)
		{			
			xs::log::warn("Data value with name '{}' is of different type.", name);
		}
	}
	else
	{
		xs::log::info("Data value '{}' not found, using default: {}.", name, default_value);
		internal::reg[name] = { type, default_value, true };
	}
	
	return default_value;
}

template<typename T>
void xs::data::internal::set(const std::string& name, const T& value, type type, bool active)
{
	internal::reg[name] = { type, value, active };
}

using namespace xs::data::internal;

void xs::data::initialize()
{
	load_of_type(type::user);
	load_of_type(type::game);
	load_of_type(type::project);
	load_of_type(type::save);
}

void xs::data::shutdown()
{
	reg.clear();
	history.clear();	
	edited.clear();
	history_stack_pointer = 0;
}

// Render inspector at a fixed rectangle (no title bar)
void xs::data::inspect()
{
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None | ImGuiTabBarFlags_DrawSelectedOverline;
	if (ImGui::BeginTabBar("DataTabsEmbedded", tab_bar_flags))
	{
		inspect_of_type("Game Data", string(ICON_FI_GAMEPAD) + " Game", type::game);
		inspect_of_type("Save Data", string(ICON_FI_SAVE) + " Save", type::save);
		inspect_of_type("Project Data", string(ICON_FI_COG) + " Project", type::project);
		ImGui::EndTabBar();
	}
}

bool xs::data::has_chages()
{
	return edited[type::game] || edited[type::project];
}

double xs::data::get_number(const std::string& name, type type, double default_value)
{
	return get<double>(name, type, default_value);
}

uint32_t xs::data::get_color(const std::string& name, type type, uint32_t default_value)
{
	return get<uint32_t>(name, type, default_value);
}

bool xs::data::get_bool(const std::string& name, type type, bool default_value)
{
	return get<bool>(name, type, default_value);
}

std::string xs::data::get_string(const std::string& name, type type, const std::string& default_value)
{
	return get<string>(name, type, default_value);
}

void xs::data::set_number(const std::string& name, double value, type tp)
{
	xs::data::internal::set<double>(name, value, tp);
}

void xs::data::set_color(const std::string& name, uint32_t value, type tp)
{
    xs::data::internal::set<uint32_t>(name, value, tp);
}

void xs::data::set_bool(const std::string& name, bool value, type tp)
{
    xs::data::internal::set<bool>(name, value, tp);
}

void xs::data::set_string(const std::string& name, const std::string& value, type tp)
{
    xs::data::internal::set<string>(name, value, tp);
}

void xs::data::save()
{
	save_of_type(type::user);
	save_of_type(type::game);
	save_of_type(type::project);
	save_of_type(type::save);
	edited.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////									Internal Impl
////////////////////////////////////////////////////////////////////////////////////////////////////
void xs::data::internal::inspect_of_type(
	const std::string& name,
	const std::string& icon,
	type type)
{	
	int flags = ImGuiTabItemFlags_None;
	if(edited[type])
		flags |= ImGuiTabItemFlags_UnsavedDocument;

	if (ImGui::BeginTabItem(icon.c_str(), NULL, flags))
	{
		tooltip(name.c_str());

		if (history.empty())
		{
			auto r(reg);
			history.push_back(r);
		}

		ImGui::BeginDisabled(!(internal::history_stack_pointer < history.size() - 1));
		if (ImGui::Button(ICON_FI_UNDO))
		{
			internal::undo();
		}
		tooltip("Undo");
		ImGui::EndDisabled();
		ImGui::SameLine();

		ImGui::BeginDisabled(internal::history_stack_pointer == 0);
		if (ImGui::Button(ICON_FI_REDO))
		{
			internal::redo();
		}
		tooltip("Redo");
		ImGui::EndDisabled();
		ImGui::SameLine();

		static ImGuiTextFilter filter;
		filter.Draw(ICON_FI_SEARCH);
		ImGui::SameLine();
		if (ImGui::Button(ICON_FI_CLEAR_FILTER)) {
			filter.Clear();
		}
		tooltip("Clear filter");

		bool ed = edited[type];
		{
			if (ed)
			{
				auto c = inspector::get_color(inspector::color_id::Purple);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(c.r, c.g, c.b, c.a));
			}
			if (ImGui::Button(string(ICON_FI_SAVE).c_str()))
				save_of_type(type);
			if (ed)
				ImGui::PopStyleColor();
			tooltip("Save to a file");
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::Text("%s", fileio::get_path(get_file_path(type)).c_str());
			ImGui::PopStyleColor();			
		}

		vector<string> sorted;
		for (auto& itr : reg)
			if (filter.PassFilter(itr.first.c_str()) && itr.second.data_type == type && itr.first != "Main")
				sorted.push_back(itr.first);
		sort(sorted.begin(), sorted.end());

		// Lambda that renders a single registry entry.
		// display_name is the short label to show in the widget (without group prefix or enum options).
		auto render_entry = [&](std::pair<const std::string, registry_value>& itr, const std::string& display_name)
		{
			bool entry_edited = false;

			ImGui::PushID(itr.first.c_str());

			{
				auto val = std::get_if<double>(&itr.second.value);
				if (val)
				{
					auto pos = itr.first.find('|');
					if (pos != string::npos)
					{
						int vint = (int)*val;
						auto enum_options_string = itr.first.substr(pos + 1);
						auto enum_options = tools::string_split(enum_options_string, "|");

						if (ImGui::BeginCombo(display_name.c_str(), enum_options[vint].c_str()))
						{
							for (size_t i = 0; i < enum_options.size(); i++)
							{
								bool is_selected = vint == (int)i;
								if (ImGui::Selectable(enum_options[i].c_str(), is_selected))
								{
									vint = (int)i;
									entry_edited = true;
								}
								if (is_selected)
									ImGui::SetItemDefaultFocus();
							}
							set(itr.first, (double)vint, itr.second.data_type, itr.second.active);
							ImGui::EndCombo();
						}
					}
					else
					{
						float flt = (float)*val;
						entry_edited = ImGui::DragFloat(display_name.c_str(), &flt, 0.01f);
						set(itr.first, (double)flt, itr.second.data_type, itr.second.active);
					}
				}
			}

			{
				auto val = std::get_if<bool>(&itr.second.value);
				if (val)
				{
					entry_edited = ImGui::Checkbox(display_name.c_str(), val);
					set(itr.first, *val, itr.second.data_type, itr.second.active);
				}
			}

			{
				auto val = std::get_if<uint32_t>(&itr.second.value);
				if (val)
				{
					ImVec4 vec = color_convert(*val);
					entry_edited = ImGui::ColorEdit4(display_name.c_str(), &vec.x);
					*val = color_convert(vec);
					set(itr.first, *val, itr.second.data_type, itr.second.active);
				}
			}

			{
				auto val = std::get_if<string>(&itr.second.value);
				if (val)
				{
					ImGui::PushItemWidth(0);
					entry_edited = ImGui::InputText(display_name.c_str(), val);
					ImGui::PopItemWidth();
					set(itr.first, *val, itr.second.data_type);
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

			if (!itr.second.active)
			{
				ImGui::SameLine();
				auto color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
				color.w = 0.5f;
				ImGui::PushStyleColor(ImGuiCol_Text, color);

				if (ImGui::Button(ICON_FI_DELETE))
				{
					reg.erase(itr.first);
					entry_edited = true;
					regsitry_type r(reg);
					history.push_back(r);
				}

				ImGui::PopStyleColor();
			}

			ImGui::PopID();

			if(entry_edited)
				edited[type] = true;
		};

		// Build recursive tree from sorted keys and render it.
		tree_node root = build_tree(sorted);

		// Recursive renderer using a self-referencing lambda.
		auto render_node = [&](auto& self, const tree_node& node) -> void {
			for (const auto& s : node.entries)
				render_entry(*reg.find(s), get_leaf_name(s));
			for (auto& [group_name, child] : node.children)
			{
				if (ImGui::TreeNodeEx(group_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					self(self, child);
					ImGui::TreePop();
				}
			}
		};

		ImGui::BeginChild("Child");
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.45f);

		render_node(render_node, root);

		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::EndTabItem();
	}
	else
	{
		tooltip(name.c_str());
	}
}

void xs::data::save_of_type(type type)
{
	nlohmann::json j;
	for (auto& itr : reg)
	{
		if (itr.second.data_type == type)
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

	edited[type] = false;
}

void xs::data::internal::load_of_type(type type)
{	
	auto filename = get_file_path(type);	
	if(fileio::exists(filename))
	{
		auto file = fileio::read_text_file(filename);
		if (!file.empty())
		{
			nlohmann::json j = nlohmann::json::parse(file);
			for (auto it = j.begin(); it != j.end(); ++it)
			{
				auto name = it.key();
				auto entry = it.value();
				auto value = entry["value"];
				auto etype = entry["type"];
				if (etype == "color")
					set_color(name, (uint32_t)value, type);
				else if (etype == "bool")
					set_bool(name, (bool)value, type);
				else if (etype == "number")
					set_number(name, (double)value, type);
				else if (etype == "string")
					set_string(name, value, type);
			}
		}
	}
	else
	{
		xs::log::info("No data file '{}' found.", filename);
	}
}

const string& xs::data::internal::get_file_path(type type)
{
	static std::string game_path = "[game]/game.json";
	static std::string save_path = "[save]/savegame.json";
	static std::string project_path = "[game]/project.json";
	static std::string user_path = "[user]/settings.json";
	static std::string no_path = "";

	switch (type)
	{
	case xs::data::type::none:
		return no_path;
	case xs::data::type::project:
		return project_path;
	case xs::data::type::game:
		return game_path;
	case xs::data::type::save:
		return save_path;
	case xs::data::type::user:
		return user_path;
	}

	return no_path;
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

