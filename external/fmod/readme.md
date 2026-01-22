# FMOD Audio Engine

This directory contains the FMOD SDK files for the xs game engine.

## Installation

### Windows

1. Download FMOD Studio API from [fmod.com](https://fmod.com/download)
   - FMOD Studio API Windows (for PC builds)
   - FMOD Studio API Switch (for Nintendo Switch builds, if available)
   - FMOD Studio API Prospero (for PlayStation 5 builds, if available)

2. Install each platform SDK to the default location:
   - `C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\`
   - `C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Switch\`
   - `C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Prospero\`

3. Run the dependency updater:
   ```bash
   python tools/update_deps/update_dependencies.py
   ```
   
4. Select "FMOD" from the list and update it. The script will automatically:
   - Copy shared headers to `external/fmod/inc/`
   - Copy Windows libraries to `external/fmod/lib/win/`
   - Copy NX libraries to `platforms/nx/external/fmod/lib/`
   - Copy Prospero libraries to `platforms/prospero/external/fmod/lib/`

### macOS

1. Download FMOD Programmers API Mac and iOS DMG files
2. Mount the DMG files
3. Run the dependency updater which will copy files from the mounted volumes

## Directory Structure

After installation, the structure should be:
```
external/fmod/
├── inc/                    # Shared header files (all platforms)
│   ├── fmod.h
│   ├── fmod.hpp
│   ├── fmod_studio.h
│   └── ...
├── lib/
│   ├── win/               # Windows x64 libraries
│   ├── mac/               # macOS libraries (if macOS build)
│   └── ios/               # iOS libraries (if iOS build)
└── readme.md

platforms/nx/external/fmod/
└── lib/                   # Nintendo Switch libraries

platforms/prospero/external/fmod/
└── lib/                   # PlayStation 5 libraries
```

## License

FMOD is proprietary software by Firelight Technologies. Please review their licensing terms at [fmod.com/licensing](https://fmod.com/licensing) before use.