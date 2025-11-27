# FreeType Integration for xs Game Engine

This directory contains the FreeType font rendering library for the xs game engine.

## Overview

FreeType is a high-quality font rendering library that provides better font quality compared to the default stb_truetype rasterizer. It is integrated with Dear ImGui to provide improved font rendering on desktop platforms.

**Platform Support:** Windows (PC) and macOS only. Console platforms (Switch, PS5) use the default stb_truetype rasterizer.

## Directory Structure

```
freetype/
├── inc/                    # Shared header files (all platforms)
│   ├── freetype/          # FreeType API headers
│   └── ft2build.h         # Main FreeType configuration header
├── lib/                    # Platform-specific libraries
│   ├── win/               # Windows (x64) static libraries
│   └── mac/               # macOS (universal binary) static libraries
├── LICENSE.TXT            # FreeType license (BSD-style)
└── README.md              # This file
```

## Building from Source

FreeType is built from source using the provided build script. This ensures compatibility across different development environments.

### Prerequisites

**Windows:**
- CMake (3.15 or later)
- Visual Studio 2019 or later (with C++ development tools)
- wget or curl (for downloading source)

**macOS:**
- Xcode Command Line Tools
- wget or curl (for downloading source)

### Build Instructions

1. **Using the dependency updater (Recommended):**
   ```bash
   cd tools/update_deps
   python update_dependencies.py
   ```
   Select FreeType from the list and run the update.

2. **Manual build:**
   ```bash
   cd tools/update_deps
   python build_freetype.py
   ```

The build script will:
- Download FreeType 2.13.2 source from the official repository
- Build static libraries optimized for your platform
- Install headers and libraries to `external/freetype/`
- Clean up temporary build files

### Build Configuration

The FreeType build is configured with minimal dependencies:
- **No zlib** - Not needed for xs use case
- **No bzip2** - Not needed for xs use case
- **No PNG** - Not needed for xs use case
- **No HarfBuzz** - Text shaping not required
- **No Brotli** - WOFF2 compression not needed
- **Static linking** - Simplifies deployment

## Using FreeType with ImGui

FreeType integration is already set up in the xs engine through ImGui's FreeType extension.

### Enabling FreeType (Optional)

To enable FreeType font rasterization, add the following to your `imconfig.h` (or before including imgui headers):

```cpp
#define IMGUI_ENABLE_FREETYPE
```

This will make ImGui automatically use FreeType instead of stb_truetype for font rendering.

### Manual Usage

If you prefer to enable FreeType manually at runtime:

```cpp
#include "imgui_freetype.h"

// After creating your ImGui context
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("path/to/font.ttf", 16.0f);

// Use FreeType rasterizer
unsigned int flags = ImGuiFreeTypeBuilderFlags_NoHinting;
ImGuiFreeType::BuildFontAtlas(io.Fonts, flags);
```

### Font Rendering Quality

FreeType provides several rendering modes:

- `ImGuiFreeTypeBuilderFlags_NoHinting` - Smooth, works well for larger fonts
- `ImGuiFreeTypeBuilderFlags_NoAutoHint` - Disable auto-hinting
- `ImGuiFreeTypeBuilderFlags_ForceAutoHint` - Force auto-hinting (good for small fonts)
- `ImGuiFreeTypeBuilderFlags_LightHinting` - Light hinting
- `ImGuiFreeTypeBuilderFlags_MonoHinting` - Monochrome hinting

Refer to `external/imgui/misc/freetype/README.md` for more details on ImGui FreeType integration.

## License

FreeType is distributed under the FreeType License (BSD-style with credit requirement). See `LICENSE.TXT` for full license text.

The FreeType license allows:
- Commercial use
- Modification
- Distribution
- Private use

Requirements:
- Include copyright notice
- Include license text
- Give credit to the FreeType Project

## Troubleshooting

### Build fails with "CMake not found"
Install CMake from https://cmake.org/ or via package manager (Windows: choco, macOS: brew)

### Build fails on Windows
Ensure Visual Studio C++ development tools are installed. Run the Visual Studio Installer and select "Desktop development with C++" workload.

### Build fails on macOS
Install Xcode Command Line Tools:
```bash
xcode-select --install
```

### Headers not found during compilation
Make sure the FreeType library was built successfully and the `external/freetype/inc/` directory exists with FreeType headers.

### Linker errors about FreeType
Verify that:
1. Static libraries exist in `external/freetype/lib/win/` (Windows) or `external/freetype/lib/mac/` (macOS)
2. The xs project file includes FreeType in the linker settings
3. You're building for a desktop platform (x64, not NX64 or Prospero)

## Version Information

- **FreeType Version:** 2.13.2
- **Build Date:** See build script output
- **Supported Platforms:** Windows (x64), macOS (universal)

## Further Reading

- [FreeType Official Website](https://www.freetype.org/)
- [FreeType Documentation](https://freetype.org/freetype2/docs/)
- [ImGui FreeType README](../imgui/misc/freetype/README.md)
- [xs Game Engine Documentation](../../docs/)
