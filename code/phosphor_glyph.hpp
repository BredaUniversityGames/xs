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

// Define the Phosphor icons used by the editor UI
// Codepoints from: external/levelscript/app/phosphor_icons.hpp (Phosphor-Bold, https://phosphoricons.com/)
// All Phosphor codepoints live in the BMP private-use range 0xE000-0xEE82, so they fit ImWchar (16-bit) directly.
// Format: X(NAME, hex_code)
#define PHOSPHOR_ICONS(X) \
    X(PLAY,                 0xe3d0) /* play */ \
    X(PAUSE,                0xe39e) /* pause */ \
    X(FAST_FORWARD,         0xe6a6) /* fast-forward */ \
    X(PROFILER,             0xe628) /* gauge */ \
    X(SYNC_ALT,             0xe094) /* arrows-clockwise */ \
    X(IMAGE,			    0xe2ca) /* image */ \
    X(BARS,				    0xe2f0) /* list */ \
    X(PIN_ON,               0xe3e2) /* push-pin */ \
    X(PIN_OFF,              0xe3e4) /* push-pin-slash */ \
    X(EXCLAMATION_CIRCLE,   0xe4e2) /* warning-circle */ \
    X(INFO_CIRCLE,          0xe2ce) /* info */ \
    X(CHECK_CIRCLE,         0xe184) /* check-circle */ \
    X(UNDO,                 0xe08a) /* arrow-u-up-left */ \
    X(REDO,                 0xe08c) /* arrow-u-up-right */ \
    X(SEARCH,               0xe30c) /* magnifying-glass */ \
    X(GAMEPAD,              0xe26e) /* game-controller */ \
    X(COG,                  0xe270) /* gear */ \
    X(SAVE,                 0xe248) /* floppy-disk */ \
    X(DELETE,               0xe4a6) /* trash */ \
    X(MEMORY,               0xe9c4) /* memory */ \
    X(IMAGE_PEN,            0xe6f0) /* paint-brush */ \
    X(IMAGES,               0xe836) /* images */ \
    X(IMAGE_FRAME,          0xe626) /* frame-corners */ \
    X(FOLDER,               0xe24a) /* folder */ \
    X(TAG,                  0xe478) /* tag */ \
    X(WINDOW, 			    0xe5da) /* app-window */ \
    X(PUZZLE_CUBE,          0xe596) /* puzzle-piece */ \
    X(CLEAR_FILTER,         0xe26c) /* funnel-x */ \

    // Add more here

// Generate string defines using proper UTF-8 encoding
#define PHOSPHOR_ICON_DEFINE(name, code) \
    constexpr auto ICON_PH_##name##_ARR = utf8_from_code(code); \
    constexpr const char ICON_PH_##name[] = { ICON_PH_##name##_ARR[0], ICON_PH_##name##_ARR[1], ICON_PH_##name##_ARR[2], ICON_PH_##name##_ARR[3], ICON_PH_##name##_ARR[4] };

PHOSPHOR_ICONS(PHOSPHOR_ICON_DEFINE)
#undef PHOSPHOR_ICON_DEFINE

// Generate glyph ranges (only the codepoints actually used, to keep the font atlas small)
namespace xs::tools
{
    inline const ImWchar* get_phosphor_glyph_ranges()
    {
        static const ImWchar ranges[] =
        {
#define PHOSPHOR_ICON_RANGE(name, code) code, code,
            PHOSPHOR_ICONS(PHOSPHOR_ICON_RANGE)
#undef PHOSPHOR_ICON_RANGE
            0  // Terminator
        };
        return ranges;
    }
}
