# XS Scripts

This directory contains utility scripts for the xs game engine.

## Dependency Updater

The `update_dependencies.py` script provides an interactive menu to manage external dependencies.

### Features

- **List all dependencies**: View all open-source and local dependencies
- **Update open-source libraries**: Fetch the latest version from their GitHub repositories
- **Check local dependencies**: View installation status for FMOD and Steam API
- **Batch updates**: Update all open-source dependencies at once

### Usage

```bash
# Basic usage (works without additional dependencies)
python scripts/update_dependencies.py

# Enhanced UI (install optional dependency first)
pip install -r scripts/requirements.txt
python scripts/update_dependencies.py
```

### Dependency Types

#### Open-Source Libraries
These can be updated automatically from their GitHub repositories:
- **imgui**: Dear ImGui - Immediate mode GUI library
- **fmt**: Modern C++ formatting library
- **glm**: OpenGL Mathematics library
- **json**: nlohmann/json - JSON for Modern C++
- **stb**: Single-file public domain libraries
- **wren**: Scripting language
- **nanosvg**: Simple SVG parser
- **miniz**: Compression library
- **dialogs**: Portable file dialogs
- **glad**: OpenGL loader

#### Local Dependencies
These require manual installation before running the updater:
- **fmod**: FMOD audio engine (PC, Nintendo Switch, PlayStation 5, Apple)
  - Download from: https://www.fmod.com/download
- **steam**: Steamworks SDK (PC only)
  - Download from: https://partner.steamgames.com/

### Notes

- The script creates backups before updating and restores them if the update fails
- Git is required for updating open-source dependencies
- Local dependencies (FMOD, Steam API) must be installed manually per platform
- The script will show platform-specific installation status for local dependencies
