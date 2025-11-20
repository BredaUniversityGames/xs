# XS Scripts

This directory contains utility scripts for the xs game engine.

## Dependency Updater

The `update_dependencies.py` script provides a modern, full-screen TUI (Terminal User Interface) to manage external dependencies using Textual.

### Features

- **Single-screen interface**: No nested menus - everything visible at once
- **Interactive table**: DataTable with clickable rows and checkbox selection
- **Real-time progress**: Progress updates shown directly in the table during updates
- **Platform auto-detection**: Automatically detects and displays your platform (Windows/macOS/Linux)
- **Theme auto-detection**: Automatically matches your terminal's light/dark theme
  - Checks `COLORFGBG` environment variable
  - On macOS: Reads system appearance preference
  - Falls back to dark theme (most common for developers)
- **Live status indicators**:
  - ✓/✗ symbols show installation status
  - ☐/☑ checkboxes for selecting dependencies
  - Progress messages during updates
- **Smart dependency management**:
  - Update multiple open-source libraries simultaneously
  - Copy FMOD from Program Files (Windows)
  - Automatic backup and restore on failures
  - Platform-aware filtering (shows only relevant dependencies)

### Usage

```bash
# Install dependencies first (only needed once)
pip install -r tools/update_deps/requirements.txt

# Run the TUI
python tools/update_deps/update_dependencies.py
```

**Logging**: All operations are logged to `tools/update_deps/dependency_updater.log`. If you miss any initial messages before the UI appears, check this log file for complete details including:
- Startup information (platform, Python version)
- Git clone operations
- File copy operations
- Success/failure messages
- Error details

### Controls

- **Mouse**: Click rows to toggle selection, click buttons to execute actions
- **Keyboard**:
  - `↑/↓` or `j/k`: Navigate table rows
  - `Space`: Toggle checkbox on current row
  - `u`: Update selected dependencies
  - `q`: Quit application
  - `Tab`: Cycle through buttons

### UI Layout

```
┌────────────────────────────────────────────────────────────────┐
│ Header: XS Dependency Updater                                  │
├────────────────────────────────────────────────────────────────┤
│ Platform: macOS (Apple) / apple                                │
├────────────────────────────────────────────────────────────────┤
│ ☐ │ Key  │ Name  │ Type │ Status    │ Description │ Progress  │
│ ☑ │ imgui│ ImGui │ Open │ ✓ Install │ Immediate.. │ ⏳ Cloning│
│ ☐ │ fmt  │ fmt   │ Open │ ○ Availab │ Modern form │           │
│ ☐ │ fmod │ FMOD  │ Local│ ✗ Not Fou │ Audio engin │           │
│ ... (all dependencies shown in scrollable table)               │
├────────────────────────────────────────────────────────────────┤
│              [Update Selected] [Exit]                          │
└────────────────────────────────────────────────────────────────┘
```

**Status Indicators:**
- `✓ Installed` - Already installed (green)
- `○ Available` - Not installed, can be auto-downloaded (cyan, for open-source)
- `✗ Not Found` - Not installed, requires manual setup (red, for local deps)

### How It Works

The updater clones each dependency into a temporary location, then copies **only the files and folders you need** to `external/{dependency}/`. This keeps your repository clean and avoids unnecessary files like tests, documentation, build scripts, etc.

Each dependency has a configured `include_paths` list that specifies:
- **Individual files**: `'imgui.cpp'`, `'LICENSE.txt'`
- **Directories**: `'include/'`, `'src/'`
- **Path mapping** (for complex cases): `('src/vm/', 'vm/')` copies `src/vm/` to `vm/`

You can customize these paths per dependency in the `DEPENDENCIES` dictionary in the script.

### Dependency Types

#### Open-Source Libraries
These can be updated automatically from their GitHub repositories:
- **imgui**: Dear ImGui - Immediate mode GUI library
- **fmt**: Modern C++ formatting library
- **glm**: OpenGL Mathematics library
- **json**: nlohmann/json - JSON for Modern C++
- **stb**: Single-file public domain libraries (image, truetype, etc.)
- **wren**: Scripting language
- **nanosvg**: Simple SVG parser
- **miniz**: Compression library
- **dialogs**: Portable file dialogs
- **glad**: OpenGL loader
- **argparse**: Argument Parser for Modern C++
- **cereal**: C++ serialization library
- **dr_libs**: Single-file audio decoding libraries (FLAC, WAV)

#### Local Dependencies
These require manual installation first, then can be updated through the UI:
- **fmod**: FMOD audio engine (PC, Nintendo Switch, PlayStation 5, Apple)
  - Download from: https://www.fmod.com/download
  - **Directory structure**:
    - Headers (shared across all platforms): `external/fmod/inc/`
    - Libraries (platform-specific): `external/fmod/lib/<platform>/`
    - Platform names: `win`, `linux`, `mac`, `ios`, `nx`, `prospero`
  - **Windows**: Auto-copies from Program Files installation
    - Headers → `external/fmod/inc/`
    - Libraries → `external/fmod/lib/win/`
  - **macOS**: Auto-copies from mounted DMG volumes
    - Mount "FMOD Programmers API Mac" DMG for macOS support
    - Mount "FMOD Programmers API iOS" DMG for iOS support
    - Can mount both and copy simultaneously
    - Headers → `external/fmod/inc/` (shared, copied once)
    - macOS libraries → `external/fmod/lib/mac/`
    - iOS libraries → `external/fmod/lib/ios/`
  - **Other platforms**: Install SDK, then manually copy:
    - Headers to `external/fmod/inc/`
    - Libraries to `external/fmod/lib/<platform>/`
- **steam**: Steamworks SDK (PC only)
  - Download from: https://partner.steamgames.com/
  - Manual installation to `external/steam/pc/` required

### Notes

- The script creates backups before updating and restores them if the update fails
- Git is required for updating open-source dependencies
- **FMOD headers are shared** across all platforms (version consistency for game assets)
- **FMOD libraries are platform-specific** and stored in separate `lib/<platform>/` directories
- **Steam API**: Must be installed and copied manually
- The script will show platform-specific installation status for local dependencies
