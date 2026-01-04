#pragma once
#include <array>
#include <imgui.h>

// Helper to convert Unicode codepoint to UTF-8 string at compile time
constexpr std::array<char, 5> utf8_from_code(unsigned int cp)
{
    std::array<char, 5> out = {0, 0, 0, 0, 0};
    if (cp <= 0x7F)
    {
        out[0] = static_cast<char>(cp & 0x7F);
        out[1] = '\0';
    }
    else if (cp <= 0x7FF)
    {
        out[0] = static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
        out[1] = static_cast<char>(0x80 | (cp & 0x3F));
        out[2] = '\0';
    }
    else if (cp <= 0xFFFF)
    {
        out[0] = static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
        out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (cp & 0x3F));
        out[3] = '\0';
    }
    else
    {
        out[0] = static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
        out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (cp & 0x3F));
        out[4] = '\0';
    }
    return out;
}

// Define all Fluent icons
// Codepoints from: https://enci.github.io/fluent-icons-cheatsheet/
// Format: X(NAME, hex_code)
#define FLUENT_ICONS(X) \
    X(PLAY,                 0xe990) \
    X(PAUSE,                0xf5a0) \
    X(FAST_FORWARD,         0xf0a2) \
    X(PROFILER,             0xe45c) \
    X(SYNC_ALT,             0xe0aa) \
    X(DATABASE,             0xf0d8) \
    X(IMAGE_RELOAD,         0xe71e) \
    X(IMAGE,			    0xf487) \
    X(BARS,				    0xf0191) \
    X(PIN_ON,               0xf600) \
    X(PIN_OFF,              0xe985) \
    X(HELP,                 0xf220) \
    X(QUESTION_CIRCLE,      0xf63c) \
    X(TIMES_CIRCLE,         0xf749) \
    X(EXCLAMATION_TRIANGLE, 0xf3f0) \
    X(INFO_CIRCLE,          0xf4a2) \
    X(CHECK_CIRCLE,         0xf297) \
    X(UNDO,                 0xe126) \
    X(REDO,                 0xe0e4) \
    X(SEARCH,               0xea7c) \
    X(TIMES,                0xf748) \
    X(GAMEPAD,              0xe689) \
    X(USER,                 0xf5bc) \
    X(BUG,                  0xe206) \
    X(COG,                  0xf6a9) \
    X(SAVE,                 0xea43) \
    X(DELETE,               0xe47b) \
    X(MEMORY,               0xf0f0) \
    X(IMAGE_PEN,            0xf492) \
    X(IMAGES,               0xe725) \
    X(IMAGE_FRAME,          0xf0053) \
    X(FOLDER,               0xe643) \
    X(TAG,                  0xf77c) \
    X(WINDOW, 			    0xee5d) \
    X(PUZZLE_CUBE,          0xf0a83) \
    // Add more here

// Generate string defines using proper UTF-8 encoding
#define FLUENT_ICON_DEFINE(name, code) \
    constexpr auto ICON_FI_##name##_ARR = utf8_from_code(code); \
    constexpr const char ICON_FI_##name[] = { ICON_FI_##name##_ARR[0], ICON_FI_##name##_ARR[1], ICON_FI_##name##_ARR[2], ICON_FI_##name##_ARR[3], ICON_FI_##name##_ARR[4] };

FLUENT_ICONS(FLUENT_ICON_DEFINE)
#undef FLUENT_ICON_DEFINE

// Generate glyph ranges
namespace xs::tools
{
    inline const ImWchar* get_fluent_glyph_ranges()
    {
        static const ImWchar ranges[] =
        {
#define FLUENT_ICON_RANGE(name, code) code, code,
            FLUENT_ICONS(FLUENT_ICON_RANGE)
#undef FLUENT_ICON_RANGE
            0  // Terminator
        };
        return ranges;
    }
}
