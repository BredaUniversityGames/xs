#!/usr/bin/env python3
"""
XS Dependency Updater
Interactive TUI to update external dependencies for the xs game engine.
"""

import os
import sys
import shutil
import subprocess
import platform
import asyncio
import logging
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

try:
    from textual.app import App, ComposeResult
    from textual.containers import Container, Horizontal, Vertical
    from textual.widgets import Header, Footer, DataTable, Button, Static, Label
    from textual.binding import Binding
    from textual.coordinate import Coordinate
    from textual import work
    from textual.worker import Worker
except ImportError:
    print("Error: textual is required. Install with: pip install -r requirements.txt")
    sys.exit(1)


# Set up logging to file
LOG_FILE = Path(__file__).parent / 'dependency_updater.log'
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(LOG_FILE, mode='w'),  # Overwrite log each run
        logging.StreamHandler(sys.stdout)  # Also print to console initially
    ]
)
logger = logging.getLogger(__name__)


# Dependency definitions
DEPENDENCIES = {
    'imgui': {
        'name': 'Dear ImGui',
        'type': 'opensource',
        'repo': 'https://github.com/ocornut/imgui.git',
        'branch': 'master',
        'description': 'Immediate mode GUI library',
        'include_paths': [
            # Core files
            'imgui.cpp',
            'imgui.h',
            'imgui_demo.cpp',
            'imgui_draw.cpp',
            'imgui_internal.h',
            'imgui_tables.cpp',
            'imgui_widgets.cpp',
            'imstb_rectpack.h',
            'imstb_textedit.h',
            'imstb_truetype.h',
            'imconfig.h',
            # Backends
            ('backends/imgui_impl_glfw.cpp', 'imgui_impl_glfw.cpp'),
            ('backends/imgui_impl_glfw.h', 'imgui_impl_glfw.h'),
            ('backends/imgui_impl_opengl3.cpp', 'imgui_impl_opengl3.cpp'),
            ('backends/imgui_impl_opengl3.h', 'imgui_impl_opengl3.h'),
            ('backends/imgui_impl_opengl3_loader.h', 'imgui_impl_opengl3_loader.h'),
            ('backends/imgui_impl_metal.h', 'imgui_impl_metal.h'),
            ('backends/imgui_impl_metal.mm', 'imgui_impl_metal.mm'),
            ('backends/imgui_impl_osx.h', 'imgui_impl_osx.h'),
            ('backends/imgui_impl_osx.mm', 'imgui_impl_osx.mm'),
            ('backends/imgui_impl_sdl3.cpp', 'imgui_impl_sdl3.cpp'),
            ('backends/imgui_impl_sdl3.h', 'imgui_impl_sdl3.h'),
            # Misc
            ('misc/cpp/imgui_stdlib.cpp', 'imgui_stdlib.cpp'),
            ('misc/cpp/imgui_stdlib.h', 'imgui_stdlib.h'),
            'LICENSE.txt',
            # Note: IconsFontAwesome*.h and imgui_impl.cpp/h are custom XS files
            # and should be preserved during updates (not overwritten)
        ],
    },
    'fmt': {
        'name': 'fmt',
        'type': 'opensource',
        'repo': 'https://github.com/fmtlib/fmt.git',
        'branch': 'master',
        'description': 'Modern formatting library',
        'include_paths': [
            'include/',
            'src/',
            'LICENSE',
        ],
    },
    'glm': {
        'name': 'GLM',
        'type': 'opensource',
        'repo': 'https://github.com/g-truc/glm.git',
        'branch': 'master',
        'description': 'OpenGL Mathematics library',
        'include_paths': [
            'glm/',
            'copying.txt',
        ],
    },
    'json': {
        'name': 'nlohmann/json',
        'type': 'opensource',
        'repo': 'https://github.com/nlohmann/json.git',
        'branch': 'develop',
        'description': 'JSON for Modern C++',
        'include_paths': [
            ('single_include/nlohmann/json.hpp', 'json.hpp'),
            'LICENSE.MIT',
        ],
    },
    'stb': {
        'name': 'stb',
        'type': 'opensource',
        'repo': 'https://github.com/nothings/stb.git',
        'branch': 'master',
        'description': 'Single-file public domain libraries',
        'include_paths': [
            'stb_image.h',
            'stb_image_write.h',
            'stb_truetype.h',
            'stb_rect_pack.h',
            'stb_vorbis.c',
            'stb_easy_font.h',
        ],
    },
    'wren': {
        'name': 'Wren',
        'type': 'opensource',
        'repo': 'https://github.com/wren-lang/wren.git',
        'branch': 'main',
        'description': 'Scripting language',
        'include_paths': [
            ('src/include/', 'include/'),
            ('src/vm/', 'vm/'),
            ('src/optional/', 'optional/'),
            'LICENSE',
        ],
    },
    'nanosvg': {
        'name': 'NanoSVG',
        'type': 'opensource',
        'repo': 'https://github.com/memononen/nanosvg.git',
        'branch': 'master',
        'description': 'Simple SVG parser',
        'include_paths': [
            ('src/nanosvg.h', 'nanosvg.h'),
            'LICENSE.txt',
        ],
    },
    'miniz': {
        'name': 'miniz',
        'type': 'opensource',
        'repo': 'https://github.com/richgel999/miniz.git',
        'branch': 'master',
        'description': 'Compression library',
        'include_paths': [
            'include/',
            'src/',
            'LICENSE',
            'readme.md',
        ],
    },
    'argparse': {
        'name': 'argparse',
        'type': 'opensource',
        'repo': 'https://github.com/p-ranav/argparse.git',
        'branch': 'master',
        'description': 'Argument Parser for Modern C++',
        'include_paths': [
            ('include/argparse/argparse.hpp', 'argparse.hpp'),
            'LICENSE',
        ],
    },
    'cereal': {
        'name': 'Cereal',
        'type': 'opensource',
        'repo': 'https://github.com/USCiLab/cereal.git',
        'branch': 'master',
        'description': 'C++ serialization library',
        'include_paths': [
            'include/cereal/',
            'LICENSE',
        ],
    },
    'dr_libs': {
        'name': 'dr_libs',
        'type': 'opensource',
        'repo': 'https://github.com/mackron/dr_libs.git',
        'branch': 'master',
        'description': 'Single-file audio decoding libraries',
        'include_paths': [
            'dr_flac.h',
            'dr_wav.h',
        ],
    },
    'dialogs': {
        'name': 'Portable File Dialogs',
        'type': 'opensource',
        'repo': 'https://github.com/samhocevar/portable-file-dialogs.git',
        'branch': 'main',
        'description': 'Cross-platform file dialogs',
        'include_paths': [
            'portable-file-dialogs.h',
            'COPYING',
        ],
    },
    'glad': {
        'name': 'GLAD',
        'type': 'opensource',
        'repo': 'https://github.com/Dav1dde/glad.git',
        'branch': 'master',
        'description': 'OpenGL loader',
        'platforms': ['pc'],  # Not needed on macOS (uses Metal)
        'include_paths': [
            'include/',
            'src/',
            'LICENSE',
        ],
    },
    'fmod': {
        'name': 'FMOD',
        'type': 'local',
        'platforms': ['pc', 'nx', 'prospero', 'apple'],
        'description': 'Audio engine (shared headers, platform libs)',
        'install_notes': 'Windows: Install to Program Files | macOS: Mount DMG | Other: Manual to inc/ and lib/<platform>/',
        # Note: update_function will be set after the function is defined
    },
    'sdl3': {
        'name': 'SDL3',
        'type': 'local',
        'platforms': ['pc', 'apple'],
        'description': 'Simple DirectMedia Layer 3 (windowing/input)',
        'install_notes': 'Windows: Download VC libs from releases | macOS: Mount DMG volumes',
        # Note: update_function will be set after the function is defined
    },
    'steam': {
        'name': 'Steam API',
        'type': 'local',
        'platforms': ['pc'],
        'description': 'Steamworks SDK (requires local installation)',
        'install_notes': 'Install Steamworks SDK from https://partner.steamgames.com/',
    },
}


def get_platform_name() -> str:
    """Get a friendly platform name."""
    plat = platform.system()
    if plat == "Windows":
        return "Windows (PC)"
    elif plat == "Darwin":
        return "macOS (Apple)"
    elif plat == "Linux":
        return "Linux (PC)"
    return plat


def get_platform_key() -> str:
    """Get the platform key for dependency matching."""
    plat = platform.system()
    if plat == "Windows" or plat == "Linux":
        return "pc"
    elif plat == "Darwin":
        return "apple"
    return "unknown"


def get_repo_root() -> Path:
    """Get the repository root directory."""
    script_dir = Path(__file__).parent  # tools/update_deps/
    return script_dir.parent.parent      # Go up two levels to repo root


def run_command(cmd: List[str], cwd: Optional[Path] = None) -> Tuple[bool, str]:
    """Run a shell command and return success status and output."""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True,
            check=False
        )
        return result.returncode == 0, result.stdout + result.stderr
    except Exception as e:
        return False, str(e)


def check_git_available() -> bool:
    """Check if git is available."""
    success, _ = run_command(['git', '--version'])
    return success


def check_dependency_available(dep_key: str, dep_info: Dict) -> bool:
    """Check if dependency source is available to copy/update from."""
    if dep_info['type'] == 'opensource':
        # Open-source is always available (can clone from git)
        return True
    elif dep_info['type'] == 'local':
        # Check if custom update function can find the source
        plat = platform.system()

        if dep_key == 'fmod':
            if plat == "Windows":
                # Check for Windows Program Files installation
                fmod_base = Path(r"C:\Program Files (x86)\FMOD SoundSystem")
                return (fmod_base / "FMOD Studio API Windows").exists()
            elif plat == "Darwin":
                # Check for mounted DMG volumes
                volumes_base = Path("/Volumes")
                mac_volume = volumes_base / "FMOD Programmers API Mac" / "FMOD Programmers API"
                ios_volume = volumes_base / "FMOD Programmers API iOS" / "FMOD Programmers API"
                return mac_volume.exists() or ios_volume.exists()
            else:
                return False

        elif dep_key == 'sdl3':
            if plat == "Darwin":
                # Check for mounted SDL DMG volumes
                volumes_base = Path("/Volumes")
                # SDL DMG naming: "SDL3" or "SDL3-3.x.x"
                # Check if any SDL3 volume is mounted
                try:
                    for volume in volumes_base.iterdir():
                        if volume.name == "SDL3" or volume.name.startswith("SDL3-"):
                            # Check if it has the expected structure
                            if (volume / "SDL3.xcframework").exists():
                                return True
                except:
                    pass
                return False
            else:
                return False

        # Other local dependencies default to not available
        return False
    return False


def check_dependency_installed(dep_key: str, dep_info: Dict) -> bool:
    """Check if a dependency is installed by looking for specific expected files."""
    repo_root = get_repo_root()
    dep_path = repo_root / 'external' / dep_key

    if dep_info['type'] == 'opensource':
        # For open-source, check if the include_paths exist
        if 'include_paths' in dep_info and dep_info['include_paths']:
            # Check if at least one of the expected files/dirs exists
            for path_item in dep_info['include_paths']:
                if isinstance(path_item, tuple):
                    # Path mapping: check destination path
                    expected_path = dep_path / path_item[1]
                else:
                    # Direct path
                    expected_path = dep_path / path_item

                if expected_path.exists():
                    return True
            return False
        else:
            # Fallback: check if directory exists and has content
            return dep_path.exists() and any(dep_path.iterdir())

    elif dep_info['type'] == 'local':
        # For local deps, check based on dependency type
        platform_key = get_platform_key()
        if platform_key not in dep_info['platforms']:
            return False

        # SDL3 and FMOD use the same structure: shared headers and platform-specific libraries
        # Structure: external/fmod/inc/ (shared) and external/fmod/lib/<platform>/
        inc_path = dep_path / 'inc'

        # For libraries, we need to map platform keys to library directory names
        # Platform key to library folder mapping
        lib_platform_map = {
            'pc': ['win', 'linux'],  # PC can have both Windows and Linux
            'apple': ['mac', 'ios'],  # Apple has mac (macOS) and ios
            'nx': ['nx'],
            'prospero': ['prospero']
        }

        # Check if headers exist (shared across all platforms)
        has_headers = inc_path.exists() and any(inc_path.iterdir())

        # Check if platform-specific libraries exist in the NEW structure
        # Must be in lib/<platform>/ subdirectories, NOT directly in lib/
        lib_base = dep_path / 'lib'
        has_libs = False
        if lib_base.exists() and lib_base.is_dir():
            # Check for any of the valid platform library folders
            lib_platforms = lib_platform_map.get(platform_key, [])
            for lib_platform in lib_platforms:
                lib_platform_path = lib_base / lib_platform
                # Must be a directory with files (not files directly in lib/)
                if lib_platform_path.exists() and lib_platform_path.is_dir():
                    try:
                        if any(lib_platform_path.iterdir()):
                            has_libs = True
                            break
                    except:
                        pass

        return has_headers and has_libs  # Both must exist

    return False


def copy_filtered_paths(src_dir: Path, dst_dir: Path, include_paths: List) -> None:
    """Copy only specified files/directories from source to destination.

    include_paths can contain:
    - string: 'path/to/file' copies src_dir/path/to/file to dst_dir/path/to/file
    - tuple: ('src/path', 'dst/path') copies src_dir/src/path to dst_dir/dst/path
    """
    for path_item in include_paths:
        # Handle path mapping (tuple) vs direct path (string)
        if isinstance(path_item, tuple):
            src_rel, dst_rel = path_item
            src_path = src_dir / src_rel
            dst_path = dst_dir / dst_rel
        else:
            src_rel = path_item
            src_path = src_dir / path_item
            dst_path = dst_dir / path_item

        if not src_path.exists():
            continue  # Skip if path doesn't exist in source

        if src_path.is_file():
            # Copy single file
            dst_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src_path, dst_path)
        elif src_path.is_dir():
            # Copy entire directory
            dst_path.parent.mkdir(parents=True, exist_ok=True)
            if dst_path.exists():
                shutil.rmtree(dst_path)
            shutil.copytree(src_path, dst_path)


def update_opensource_dependency(dep_key: str, dep_info: Dict, progress_callback=None) -> Tuple[bool, str]:
    """Update an open-source dependency from its git repository."""
    logger.info(f"Starting update for {dep_key} ({dep_info['name']})")
    repo_root = get_repo_root()
    dep_path = repo_root / 'external' / dep_key
    temp_dir = repo_root / 'external' / f'.temp_{dep_key}'

    try:
        # Clone the repository
        if progress_callback:
            progress_callback("Cloning repository...")
        logger.info(f"Cloning {dep_info['repo']} (branch: {dep_info['branch']})")

        success, output = run_command([
            'git', 'clone',
            '--depth', '1',
            '--branch', dep_info['branch'],
            dep_info['repo'],
            str(temp_dir)
        ])

        if not success:
            logger.error(f"Clone failed for {dep_key}: {output}")
            return False, f"Failed to clone: {output}"

        # Remove .git directory from clone
        git_dir = temp_dir / '.git'
        if git_dir.exists():
            shutil.rmtree(git_dir)

        # Backup existing dependency
        backup_path = None
        if dep_path.exists():
            if progress_callback:
                progress_callback("Backing up...")
            logger.info(f"Creating backup for {dep_key}")
            backup_path = repo_root / 'external' / f'.backup_{dep_key}'
            if backup_path.exists():
                shutil.rmtree(backup_path)
            shutil.move(str(dep_path), str(backup_path))

        # Copy filtered files/directories
        if progress_callback:
            progress_callback("Copying files...")
        logger.info(f"Copying filtered files for {dep_key}")

        # Create destination directory
        dep_path.mkdir(parents=True, exist_ok=True)

        # Check if include_paths is specified
        if 'include_paths' in dep_info and dep_info['include_paths']:
            # Copy only specified paths
            logger.info(f"Using filtered copy ({len(dep_info['include_paths'])} paths specified)")
            copy_filtered_paths(temp_dir, dep_path, dep_info['include_paths'])
        else:
            # No filter - copy everything (old behavior)
            logger.info("Copying all files (no filter specified)")
            shutil.copytree(temp_dir, dep_path, dirs_exist_ok=True)

        # Clean up temp directory
        if temp_dir.exists():
            shutil.rmtree(temp_dir)

        # Remove backup if successful
        if backup_path and backup_path.exists():
            logger.info(f"Removing backup for {dep_key}")
            shutil.rmtree(backup_path)

        if progress_callback:
            progress_callback("Complete")

        logger.info(f"Successfully updated {dep_key}")
        return True, "Successfully updated"

    except Exception as e:
        logger.error(f"Error updating {dep_key}: {str(e)}")
        # Restore backup if it exists
        backup_path = repo_root / 'external' / f'.backup_{dep_key}'
        if backup_path and backup_path.exists():
            logger.info(f"Restoring backup for {dep_key}")
            if dep_path.exists():
                shutil.rmtree(dep_path)
            shutil.move(str(backup_path), str(dep_path))

        # Clean up temp directory
        if temp_dir.exists():
            shutil.rmtree(temp_dir)

        return False, str(e)


def copy_fmod_from_program_files(progress_callback=None) -> Tuple[bool, str]:
    """Copy FMOD files from Program Files installation to external/fmod."""
    logger.info("Starting FMOD copy operation")
    repo_root = get_repo_root()
    plat = platform.system()

    if plat == "Windows":
        logger.info("Platform: Windows - copying from Program Files")
        fmod_base = Path(r"C:\Program Files (x86)\FMOD SoundSystem")
        fmod_windows = fmod_base / "FMOD Studio API Windows"

        if not fmod_windows.exists():
            logger.error(f"FMOD not found at {fmod_windows}")
            return False, f"FMOD not found at {fmod_windows}"

        try:
            if progress_callback:
                progress_callback("Copying headers...")
            logger.info("Copying FMOD headers and libraries")

            # Source paths
            core_inc = fmod_windows / "api" / "core" / "inc"
            core_lib = fmod_windows / "api" / "core" / "lib" / "x64"
            studio_inc = fmod_windows / "api" / "studio" / "inc"
            studio_lib = fmod_windows / "api" / "studio" / "lib" / "x64"

            # Destination paths - shared headers, platform-specific libs
            fmod_dest = repo_root / "external" / "fmod"
            inc_dest = fmod_dest / "inc"  # Shared headers
            lib_dest = fmod_dest / "lib" / "win"  # Windows-specific libraries

            # Create destination directories
            inc_dest.mkdir(parents=True, exist_ok=True)
            lib_dest.mkdir(parents=True, exist_ok=True)

            # Copy include files (shared across all platforms)
            if core_inc.exists():
                shutil.copytree(core_inc, inc_dest, dirs_exist_ok=True)
            if studio_inc.exists():
                shutil.copytree(studio_inc, inc_dest, dirs_exist_ok=True)

            if progress_callback:
                progress_callback("Copying libraries...")

            # Copy lib files (Windows-specific)
            if core_lib.exists():
                shutil.copytree(core_lib, lib_dest, dirs_exist_ok=True)
            if studio_lib.exists():
                shutil.copytree(studio_lib, lib_dest, dirs_exist_ok=True)

            if progress_callback:
                progress_callback("Complete")

            logger.info("Successfully copied FMOD for Windows")
            return True, "Successfully copied FMOD"

        except Exception as e:
            logger.error(f"Error copying FMOD on Windows: {str(e)}")
            return False, str(e)

    elif plat == "Darwin":
        logger.info("Platform: macOS - checking for mounted DMG volumes")
        # Check for mounted FMOD DMG volumes
        volumes_base = Path("/Volumes")
        mac_volume = volumes_base / "FMOD Programmers API Mac" / "FMOD Programmers API"
        ios_volume = volumes_base / "FMOD Programmers API iOS" / "FMOD Programmers API"

        # Check what volumes are available
        has_mac = mac_volume.exists()
        has_ios = ios_volume.exists()

        if not has_mac and not has_ios:
            logger.error("No FMOD DMG mounted")
            return False, "FMOD DMG not mounted. Please mount FMOD Programmers API Mac or iOS DMG"

        try:
            repo_root = get_repo_root()
            fmod_dest = repo_root / "external" / "fmod"
            inc_dest = fmod_dest / "inc"  # Shared headers

            # Create shared headers directory
            inc_dest.mkdir(parents=True, exist_ok=True)

            copied_platforms = []

            # Copy from macOS volume if available
            if has_mac:
                logger.info(f"Found macOS FMOD volume at {mac_volume}")
                if progress_callback:
                    progress_callback("Copying macOS FMOD...")

                core_inc = mac_volume / "api" / "core" / "inc"
                core_lib = mac_volume / "api" / "core" / "lib"
                studio_inc = mac_volume / "api" / "studio" / "inc"
                studio_lib = mac_volume / "api" / "studio" / "lib"

                # Copy headers (shared, only need to do once)
                logger.info("Copying shared FMOD headers")
                if core_inc.exists():
                    shutil.copytree(core_inc, inc_dest, dirs_exist_ok=True)
                if studio_inc.exists():
                    shutil.copytree(studio_inc, inc_dest, dirs_exist_ok=True)

                # Copy macOS-specific libraries
                logger.info("Copying macOS libraries to lib/mac/")
                lib_dest = fmod_dest / "lib" / "mac"
                lib_dest.mkdir(parents=True, exist_ok=True)
                if core_lib.exists():
                    shutil.copytree(core_lib, lib_dest, dirs_exist_ok=True)
                if studio_lib.exists():
                    shutil.copytree(studio_lib, lib_dest, dirs_exist_ok=True)

                copied_platforms.append("macOS")

            # Copy from iOS volume if available
            if has_ios:
                logger.info(f"Found iOS FMOD volume at {ios_volume}")
                if progress_callback:
                    progress_callback("Copying iOS FMOD...")

                core_inc = ios_volume / "api" / "core" / "inc"
                core_lib = ios_volume / "api" / "core" / "lib"
                studio_inc = ios_volume / "api" / "studio" / "inc"
                studio_lib = ios_volume / "api" / "studio" / "lib"

                # Copy headers if we haven't already (shared)
                if not has_mac:
                    logger.info("Copying shared FMOD headers")
                    if core_inc.exists():
                        shutil.copytree(core_inc, inc_dest, dirs_exist_ok=True)
                    if studio_inc.exists():
                        shutil.copytree(studio_inc, inc_dest, dirs_exist_ok=True)

                # Copy iOS-specific libraries
                logger.info("Copying iOS libraries to lib/ios/")
                lib_dest = fmod_dest / "lib" / "ios"
                lib_dest.mkdir(parents=True, exist_ok=True)
                if core_lib.exists():
                    shutil.copytree(core_lib, lib_dest, dirs_exist_ok=True)
                if studio_lib.exists():
                    shutil.copytree(studio_lib, lib_dest, dirs_exist_ok=True)

                copied_platforms.append("iOS")

            if progress_callback:
                progress_callback("Complete")

            platforms_str = " and ".join(copied_platforms)
            logger.info(f"Successfully copied {platforms_str} FMOD")
            return True, f"Successfully copied {platforms_str} FMOD"

        except Exception as e:
            logger.error(f"Error copying FMOD on macOS: {str(e)}")
            return False, f"Error copying FMOD: {e}"

    elif plat == "Linux":
        logger.warning("FMOD copy on Linux not implemented")
        return False, "FMOD copy on Linux not implemented. Please copy headers to external/fmod/inc/ and libraries to external/fmod/lib/linux/"

    else:
        logger.error(f"Unsupported platform: {plat}")
        return False, f"Unsupported platform: {plat}"


def copy_sdl3_from_dmg(progress_callback=None) -> Tuple[bool, str]:
    """Copy SDL3 from mounted DMG volumes to external/sdl3."""
    logger.info("Starting SDL3 copy operation")
    repo_root = get_repo_root()
    plat = platform.system()

    if plat == "Darwin":
        logger.info("Platform: macOS - checking for mounted SDL3 DMG volumes")

        # Find SDL3 volume
        volumes_base = Path("/Volumes")
        sdl_volume = None

        try:
            for volume in volumes_base.iterdir():
                if volume.name == "SDL3" or volume.name.startswith("SDL3-"):
                    # Check for xcframework (universal binary for macOS/iOS)
                    if (volume / "SDL3.xcframework").exists():
                        sdl_volume = volume
                        logger.info(f"Found SDL3 volume at {volume}")
                        break
        except Exception as e:
            logger.error(f"Error scanning volumes: {str(e)}")
            return False, f"Error scanning for SDL3 DMG: {str(e)}"

        if not sdl_volume:
            logger.error("No SDL3 DMG mounted")
            return False, "SDL3 DMG not mounted. Please mount SDL3 DMG from https://github.com/libsdl-org/SDL/releases"

        try:
            if progress_callback:
                progress_callback("Copying SDL3...")

            sdl_dest = repo_root / "external" / "sdl3"
            inc_dest = sdl_dest / "inc"
            lib_dest = sdl_dest / "lib"

            # Create directories
            sdl_dest.mkdir(parents=True, exist_ok=True)
            inc_dest.mkdir(parents=True, exist_ok=True)

            # Copy LICENSE
            license_src = sdl_volume / "LICENSE.txt"
            if license_src.exists():
                logger.info("Copying LICENSE.txt")
                shutil.copy2(license_src, sdl_dest / "LICENSE.txt")

            # Define xcframework path
            xcframework_path = sdl_volume / "SDL3.xcframework"

            # Copy the entire xcframework (useful for Xcode projects)
            frameworks_dest = sdl_dest / "frameworks"
            frameworks_dest.mkdir(parents=True, exist_ok=True)
            xcframework_dest = frameworks_dest / "SDL3.xcframework"
            logger.info("Copying SDL3.xcframework to frameworks/")
            if progress_callback:
                progress_callback("Copying xcframework...")
            if xcframework_dest.exists():
                shutil.rmtree(xcframework_dest)
            shutil.copytree(xcframework_path, xcframework_dest)

            # Also extract headers and binaries for convenience
            # xcframework contains platform-specific directories like:
            # - macos-arm64_x86_64/SDL3.framework/
            # - ios-arm64/SDL3.framework/

            # Find the macOS and iOS frameworks inside xcframework
            macos_framework = None
            ios_framework = None

            for item in xcframework_path.iterdir():
                if item.is_dir():
                    # Check directory name to determine platform
                    if "macos" in item.name.lower():
                        framework_path = item / "SDL3.framework"
                        if framework_path.exists():
                            macos_framework = framework_path
                            logger.info(f"Found macOS framework at {item.name}")
                    elif "ios-arm64" == item.name.lower() and "simulator" not in item.name.lower():
                        # Get real iOS device framework, not simulator
                        framework_path = item / "SDL3.framework"
                        if framework_path.exists():
                            ios_framework = framework_path
                            logger.info(f"Found iOS framework at {item.name}")

            # Copy shared headers (from either framework, they're the same)
            source_framework = macos_framework if macos_framework else ios_framework
            if source_framework:
                headers_src = source_framework / "Headers"
                if headers_src.exists():
                    logger.info("Copying shared SDL3 headers")
                    shutil.copytree(headers_src, inc_dest, dirs_exist_ok=True)

            if progress_callback:
                progress_callback("Copying libraries...")

            copied_platforms = []

            # Copy macOS libraries
            if macos_framework:
                logger.info("Copying macOS libraries to lib/mac/")
                lib_mac_dest = lib_dest / "mac"
                lib_mac_dest.mkdir(parents=True, exist_ok=True)

                # Copy the framework binary
                dylib_src = macos_framework / "SDL3"
                if dylib_src.exists():
                    shutil.copy2(dylib_src, lib_mac_dest / "libSDL3.dylib")
                copied_platforms.append("macOS")

            # Copy iOS libraries
            if ios_framework:
                logger.info("Copying iOS libraries to lib/ios/")
                lib_ios_dest = lib_dest / "ios"
                lib_ios_dest.mkdir(parents=True, exist_ok=True)

                # Copy the framework binary
                lib_src = ios_framework / "SDL3"
                if lib_src.exists():
                    shutil.copy2(lib_src, lib_ios_dest / "libSDL3.a")
                copied_platforms.append("iOS")

            if progress_callback:
                progress_callback("Complete")

            if copied_platforms:
                platforms_str = " and ".join(copied_platforms)
                logger.info(f"Successfully copied {platforms_str} SDL3")
                return True, f"Successfully copied {platforms_str} SDL3"
            else:
                logger.error("No platforms found in xcframework")
                return False, "Could not find macOS or iOS frameworks in xcframework"

        except Exception as e:
            logger.error(f"Error copying SDL3 on macOS: {str(e)}")
            return False, f"Error copying SDL3: {e}"

    else:
        logger.error(f"SDL3 auto-copy only supported on macOS (platform: {plat})")
        return False, "SDL3 auto-copy only supported on macOS. On Windows, manually extract to external/sdl3/"


# Set function references for dependencies with custom update handlers
# This must be done after the functions are defined
DEPENDENCIES['fmod']['update_function'] = copy_fmod_from_program_files
DEPENDENCIES['sdl3']['update_function'] = copy_sdl3_from_dmg


class DependencyUpdaterApp(App):
    """Textual app for managing XS dependencies."""

    # Auto-detect theme from terminal
    # Textual will use DARK mode by default, or follow terminal theme if detectable
    CSS = """
    Screen {
        layout: vertical;
    }

    #platform-info {
        height: 3;
        content-align: center middle;
        background: $primary;
        color: $text;
        text-style: bold;
    }

    #main-container {
        height: 1fr;
        padding: 1;
    }

    DataTable {
        height: 1fr;
    }

    #button-bar {
        height: 5;
        dock: bottom;
        background: $panel;
        padding: 1;
        align: center middle;
    }

    Button {
        margin: 0 1;
        min-width: 16;
    }

    .success {
        color: $success;
    }

    .error {
        color: $error;
    }

    .warning {
        color: $warning;
    }
    """

    BINDINGS = [
        Binding("q", "quit", "Quit"),
        Binding("u", "update_selected", "Update Selected"),
        Binding("space", "toggle_selection", "Toggle"),
    ]

    def __init__(self):
        super().__init__()
        self.selected_deps = set()
        self.updating = False

        # Detect and set theme based on terminal
        # Textual supports automatic theme detection via TERM_PROGRAM and other env vars
        # But we can also explicitly check COLORFGBG for light/dark detection
        self._detect_terminal_theme()

    def _detect_terminal_theme(self) -> None:
        """Detect if terminal is using light or dark theme."""
        # Check COLORFGBG environment variable (used by many terminals)
        # Format is typically "foreground;background" where high values = light, low = dark
        colorfgbg = os.environ.get('COLORFGBG', '')

        if colorfgbg:
            try:
                # Extract background color value
                parts = colorfgbg.split(';')
                if len(parts) >= 2:
                    bg_color = int(parts[-1])
                    # Background color > 7 typically means light theme
                    if bg_color >= 7:
                        self.theme = "textual-light"
                        return
            except (ValueError, IndexError):
                pass

        # Check macOS appearance (via defaults command)
        if platform.system() == "Darwin":
            try:
                result = subprocess.run(
                    ['defaults', 'read', '-g', 'AppleInterfaceStyle'],
                    capture_output=True,
                    text=True,
                    timeout=1
                )
                if result.returncode == 0 and 'Dark' in result.stdout:
                    self.theme = "textual-dark"
                    return
                elif result.returncode != 0:
                    # Command failed = light mode (Dark mode not set)
                    self.theme = "textual-light"
                    return
            except (subprocess.TimeoutExpired, FileNotFoundError):
                pass

        # Default to dark theme if detection fails
        # (Most terminals and developer environments use dark themes)
        self.theme = "textual-dark"

    def compose(self) -> ComposeResult:
        """Create child widgets."""
        platform_name = get_platform_name()
        platform_key = get_platform_key()

        yield Header()
        yield Static(
            f"XS Dependency Updater - Platform: {platform_name} ({platform_key})",
            id="platform-info"
        )

        with Container(id="main-container"):
            yield DataTable(id="dep-table", cursor_type="row")

        with Horizontal(id="button-bar"):
            yield Button("Update Selected", id="update-btn", variant="primary")
            yield Button("Exit", id="exit-btn", variant="error")

        yield Footer()

    def on_mount(self) -> None:
        """Set up the table when the app starts."""
        table = self.query_one("#dep-table", DataTable)

        # Add columns
        table.add_column("☐", width=3, key="selected")
        table.add_column("Key", width=12, key="key")
        table.add_column("Name", width=22, key="name")
        table.add_column("Type", width=6, key="type")
        table.add_column("📁", width=2, key="installed")
        table.add_column("🔎", width=2, key="available")
        table.add_column("Description", width=50, key="description")
        table.add_column("Progress", width=25, key="progress")

        # Check if git is available
        if not check_git_available():
            self.notify("Git is not available!", severity="error")

        # Populate table with dependencies
        self.refresh_table()

    def refresh_table(self) -> None:
        """Refresh the table with current dependency data."""
        table = self.query_one("#dep-table", DataTable)
        table.clear()

        platform_key = get_platform_key()

        for dep_key, dep_info in DEPENDENCIES.items():
            # Check if dependency is platform-specific
            if 'platforms' in dep_info:
                # Skip if not applicable to current platform
                if platform_key not in dep_info['platforms']:
                    continue

            installed = check_dependency_installed(dep_key, dep_info)
            available = check_dependency_available(dep_key, dep_info)

            # Determine type string
            type_str = "Remote" if dep_info['type'] == 'opensource' else "Local"

            # Installed column: simple checkmark/cross
            installed_str = "[green]✓[/green]" if installed else "[red]✗[/red]"

            # Available column: simple checkmark/cross
            available_str = "[green]✓[/green]" if available else "[red]✗[/red]"

            # Checkbox state
            checkbox = "☑" if dep_key in self.selected_deps else "☐"

            # Add row
            table.add_row(
                checkbox,
                dep_key,
                dep_info['name'],
                type_str,
                installed_str,
                available_str,
                dep_info['description'],
                "",  # Progress column (empty initially)
                key=dep_key
            )

    def on_data_table_row_selected(self, event: DataTable.RowSelected) -> None:
        """Handle row selection to toggle checkbox."""
        if self.updating:
            return  # Don't allow selection changes during update

        dep_key = event.row_key.value

        # Toggle selection
        if dep_key in self.selected_deps:
            self.selected_deps.remove(dep_key)
        else:
            self.selected_deps.add(dep_key)

        # Refresh table to update checkbox
        self.refresh_table()

    def action_toggle_selection(self) -> None:
        """Toggle selection of the current row."""
        table = self.query_one("#dep-table", DataTable)
        if table.cursor_row >= 0:
            row_key = table.get_row_at(table.cursor_row)[0]
            dep_key = list(DEPENDENCIES.keys())[table.cursor_row]

            if dep_key in self.selected_deps:
                self.selected_deps.remove(dep_key)
            else:
                self.selected_deps.add(dep_key)

            self.refresh_table()

    def on_button_pressed(self, event: Button.Pressed) -> None:
        """Handle button clicks."""
        if event.button.id == "exit-btn":
            self.exit()
        elif event.button.id == "update-btn":
            self.action_update_selected()

    def action_update_selected(self) -> None:
        """Update the selected dependencies."""
        if self.updating:
            self.notify("Update already in progress", severity="warning")
            return

        if not self.selected_deps:
            self.notify("No dependencies selected", severity="warning")
            return

        # Filter updatable dependencies (opensource + local with update_function)
        updatable_selected = [
            dep_key for dep_key in self.selected_deps
            if DEPENDENCIES[dep_key]['type'] == 'opensource' or 'update_function' in DEPENDENCIES[dep_key]
        ]

        if not updatable_selected:
            self.notify("No updatable dependencies selected", severity="warning")
            return

        self.updating = True
        self.notify(f"Updating {len(updatable_selected)} dependencies...", severity="information")

        # Start update process
        self.update_dependencies(updatable_selected)

    @work(exclusive=True, thread=True)
    def update_dependencies(self, dep_keys: List[str]) -> None:
        """Update dependencies in a worker thread."""
        logger.info(f"Starting batch update for {len(dep_keys)} dependencies: {', '.join(dep_keys)}")

        for dep_key in dep_keys:
            dep_info = DEPENDENCIES[dep_key]
            logger.info(f"Processing {dep_key}...")

            # Update progress in table
            self.call_from_thread(self.update_progress, dep_key, "⏳ Starting...")

            def progress_callback(msg):
                self.call_from_thread(self.update_progress, dep_key, f"⏳ {msg}")

            # Check if dependency has a custom update function
            if 'update_function' in dep_info:
                # Call custom update function directly (function reference)
                logger.info(f"{dep_key} using custom update function")
                update_func = dep_info['update_function']
                success, message = update_func(progress_callback)
            else:
                # Default opensource update
                logger.info(f"{dep_key} using standard open-source update")
                success, message = update_opensource_dependency(dep_key, dep_info, progress_callback)

            if success:
                logger.info(f"✓ {dep_key} completed successfully")
                self.call_from_thread(self.update_progress, dep_key, "[green]✓ Complete[/green]")
                self.call_from_thread(self.notify, f"✓ {dep_key} updated successfully", severity="information")
            else:
                logger.error(f"✗ {dep_key} failed: {message}")
                self.call_from_thread(self.update_progress, dep_key, f"[red]✗ Failed: {message}[/red]")
                self.call_from_thread(self.notify, f"✗ {dep_key} failed: {message}", severity="error")

        # Done
        logger.info("Batch update complete")
        self.call_from_thread(self.finish_update)

    def update_progress(self, dep_key: str, progress_text: str) -> None:
        """Update the progress column for a dependency."""
        table = self.query_one("#dep-table", DataTable)

        # Find the row index for this dependency (accounting for filtered dependencies)
        platform_key = get_platform_key()
        row_idx = 0
        for key, info in DEPENDENCIES.items():
            # Skip dependencies not shown due to platform filtering
            if 'platforms' in info and platform_key not in info['platforms']:
                continue

            if key == dep_key:
                break
            row_idx += 1

        # Update the progress column (index 7: checkbox, key, name, type, installed, available, description, progress)
        table.update_cell_at(Coordinate(row_idx, 7), progress_text)

    def finish_update(self) -> None:
        """Called when all updates are complete."""
        self.updating = False
        self.refresh_table()
        self.notify("All updates complete!", severity="information")


def main():
    """Main entry point."""
    logger.info("=" * 60)
    logger.info("XS Dependency Updater Starting")
    logger.info(f"Platform: {platform.system()} ({platform.machine()})")
    logger.info(f"Python: {sys.version}")
    logger.info(f"Log file: {LOG_FILE}")
    logger.info("=" * 60)

    # Remove console handler before starting UI (keep file handler)
    # This prevents log messages from interfering with the TUI
    for handler in logging.root.handlers[:]:
        if isinstance(handler, logging.StreamHandler) and handler.stream == sys.stdout:
            logging.root.removeHandler(handler)

    app = DependencyUpdaterApp()
    # Textual auto-detects light/dark mode from terminal
    # You can override with: app.theme = "textual-light" or "textual-dark"
    app.run()


if __name__ == '__main__':
    main()
