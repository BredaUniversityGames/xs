#!/usr/bin/env python3
"""
Build FreeType from source for the xs game engine.
Builds for desktop platforms (Windows, macOS) and installs to external/freetype/.
"""

import os
import sys
import shutil
import subprocess
import platform
from pathlib import Path
from typing import Optional

# Version of FreeType to build
FREETYPE_VERSION = "2.13.2"
FREETYPE_URL = f"https://download.savannah.gnu.org/releases/freetype/freetype-{FREETYPE_VERSION}.tar.gz"
FREETYPE_ARCHIVE = f"freetype-{FREETYPE_VERSION}.tar.gz"
FREETYPE_DIR = f"freetype-{FREETYPE_VERSION}"


def get_repo_root() -> Path:
    """Get the repository root directory."""
    script_dir = Path(__file__).parent  # tools/update_deps/
    return script_dir.parent.parent      # Go up two levels to repo root


def run_command(cmd, cwd=None, env=None):
    """Run a shell command and return success status."""
    print(f"Running: {' '.join(cmd)}")
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            check=True,
            capture_output=True,
            text=True
        )
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error: {e.stderr}")
        return False


def download_freetype(temp_dir: Path) -> bool:
    """Download FreeType source archive."""
    print(f"Downloading FreeType {FREETYPE_VERSION}...")
    
    archive_path = temp_dir / FREETYPE_ARCHIVE
    
    # Try wget first, then curl
    if shutil.which("wget"):
        return run_command(["wget", "-O", str(archive_path), FREETYPE_URL])
    elif shutil.which("curl"):
        return run_command(["curl", "-L", "-o", str(archive_path), FREETYPE_URL])
    else:
        print("Error: Neither wget nor curl found. Please install one of them.")
        return False


def extract_archive(temp_dir: Path) -> bool:
    """Extract FreeType source archive."""
    print(f"Extracting {FREETYPE_ARCHIVE}...")
    
    archive_path = temp_dir / FREETYPE_ARCHIVE
    
    # Use tar (available on Windows 10+ and Unix systems)
    if platform.system() == "Windows":
        # Windows 10+ has built-in tar
        return run_command(["tar", "-xzf", str(archive_path), "-C", str(temp_dir)])
    else:
        return run_command(["tar", "-xzf", str(archive_path)], cwd=temp_dir)


def find_visual_studio() -> Optional[Path]:
    """Find Visual Studio installation using vswhere."""
    vswhere_path = Path(r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")
    
    if not vswhere_path.exists():
        return None
    
    try:
        result = subprocess.run(
            [str(vswhere_path), "-latest", "-property", "installationPath"],
            capture_output=True,
            text=True,
            check=True
        )
        vs_path = Path(result.stdout.strip())
        if vs_path.exists():
            return vs_path
    except:
        pass
    
    return None


def build_freetype_windows_msbuild(source_dir: Path, install_dir: Path) -> bool:
    """Build FreeType on Windows using MSBuild (Visual Studio)."""
    print("Building FreeType for Windows using MSBuild...")
    
    # Find Visual Studio
    vs_path = find_visual_studio()
    if not vs_path:
        print("Error: Visual Studio not found")
        return False
    
    # Find MSBuild
    msbuild_path = vs_path / "MSBuild" / "Current" / "Bin" / "MSBuild.exe"
    if not msbuild_path.exists():
        # Try older path
        msbuild_path = vs_path / "MSBuild" / "15.0" / "Bin" / "MSBuild.exe"
    
    if not msbuild_path.exists():
        print(f"Error: MSBuild not found in Visual Studio installation")
        return False
    
    print(f"Using MSBuild: {msbuild_path}")
    
    # FreeType has Visual Studio solution files
    builds_dir = source_dir / "builds" / "windows" / "vc2010"
    sln_file = builds_dir / "freetype.sln"
    
    if not sln_file.exists():
        print(f"Error: Solution file not found at {sln_file}")
        return False
    
    # Build using MSBuild
    if not run_command([
        str(msbuild_path),
        str(sln_file),
        "/p:Configuration=Release",
        "/p:Platform=x64",
        "/p:PlatformToolset=v143",
        "/m"
    ], cwd=builds_dir):
        return False
    
    # Copy outputs to install directory
    print("Installing FreeType...")
    
    # Create install directories
    inc_dest = install_dir / "include" / "freetype2"
    lib_dest = install_dir / "lib"
    inc_dest.mkdir(parents=True, exist_ok=True)
    lib_dest.mkdir(parents=True, exist_ok=True)
    
    # Copy headers
    include_src = source_dir / "include"
    if include_src.exists():
        shutil.copytree(include_src, inc_dest, dirs_exist_ok=True)
    
    # Copy library (look in objs directory)
    objs_dir = source_dir / "objs" / "x64" / "Release"
    if not objs_dir.exists():
        objs_dir = source_dir / "objs" / "Release"
    
    dll_copied = False
    for lib_file in objs_dir.glob("*.lib"):
        shutil.copy2(lib_file, lib_dest / lib_file.name)
    # Copy the DLL (runtime) alongside libs to allow dynamic linking fallback
    for dll_file in objs_dir.glob("*.dll"):
        shutil.copy2(dll_file, lib_dest / dll_file.name)
        dll_copied = True
    if not dll_copied:
        # Some FreeType builds place DLL in root objs directory
        root_dll = source_dir / "objs" / "freetype.dll"
        if root_dll.exists():
            shutil.copy2(root_dll, lib_dest / root_dll.name)
    
    return True


def build_freetype_windows(source_dir: Path, install_dir: Path) -> bool:
    """Build FreeType on Windows using CMake or MSBuild."""
    print("Building FreeType for Windows...")
    
    # Try CMake first (preferred)
    if shutil.which("cmake"):
        print("Using CMake build...")
        build_dir = source_dir / "build"
        build_dir.mkdir(exist_ok=True)
        
        # Configure with CMake
        cmake_args = [
            "cmake",
            "..",
            f"-DCMAKE_INSTALL_PREFIX={install_dir}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DFT_DISABLE_ZLIB=TRUE",
            "-DFT_DISABLE_BZIP2=TRUE",
            "-DFT_DISABLE_PNG=TRUE",
            "-DFT_DISABLE_HARFBUZZ=TRUE",
            "-DFT_DISABLE_BROTLI=TRUE",
            "-DBUILD_SHARED_LIBS=OFF",
        ]
        
        if not run_command(cmake_args, cwd=build_dir):
            print("CMake configuration failed, trying MSBuild...")
            return build_freetype_windows_msbuild(source_dir, install_dir)
        
        # Build
        if not run_command(["cmake", "--build", ".", "--config", "Release"], cwd=build_dir):
            return False
        
        # Install
        if not run_command(["cmake", "--install", ".", "--config", "Release"], cwd=build_dir):
            return False
        
        return True
    else:
        # Fall back to MSBuild
        print("CMake not found, using MSBuild...")
        return build_freetype_windows_msbuild(source_dir, install_dir)


def build_freetype_macos(source_dir: Path, install_dir: Path) -> bool:
    """Build FreeType on macOS using configure."""
    print("Building FreeType for macOS...")
    
    # Configure for universal binary (x86_64 + arm64)
    configure_args = [
        "./configure",
        f"--prefix={install_dir}",
        "--without-zlib",
        "--without-bzip2",
        "--without-png",
        "--without-harfbuzz",
        "--without-brotli",
        "--enable-static",
        "--disable-shared",
        "CFLAGS=-arch x86_64 -arch arm64",
        "LDFLAGS=-arch x86_64 -arch arm64",
    ]
    
    if not run_command(configure_args, cwd=source_dir):
        return False
    
    # Build
    if not run_command(["make", "-j4"], cwd=source_dir):
        return False
    
    # Install
    if not run_command(["make", "install"], cwd=source_dir):
        return False
    
    return True


def build_freetype_ios(source_dir: Path, install_dir: Path) -> bool:
    """Build FreeType for iOS using configure (cross-compile from macOS)."""
    print("Building FreeType for iOS...")

    # Get the iOS SDK path
    try:
        result = subprocess.run(
            ["xcrun", "--sdk", "iphoneos", "--show-sdk-path"],
            capture_output=True,
            text=True,
            check=True
        )
        ios_sdk = result.stdout.strip()
    except subprocess.CalledProcessError:
        print("Error: Could not find iOS SDK. Make sure Xcode is installed.")
        return False

    print(f"Using iOS SDK: {ios_sdk}")

    # Configure for iOS (arm64)
    cc = f"{ios_sdk}/usr/bin/clang"
    configure_args = [
        "./configure",
        f"--prefix={install_dir}",
        f"--host=aarch64-apple-darwin",
        "--without-zlib",
        "--without-bzip2",
        "--without-png",
        "--without-harfbuzz",
        "--without-brotli",
        "--enable-static",
        "--disable-shared",
        f"CC={cc}",
        f"CFLAGS=-arch arm64 -isysroot {ios_sdk} -miphoneos-version-min=11.0",
        f"LDFLAGS=-arch arm64 -isysroot {ios_sdk}",
    ]

    if not run_command(configure_args, cwd=source_dir):
        return False

    # Build
    if not run_command(["make", "-j4"], cwd=source_dir):
        return False

    # Install
    if not run_command(["make", "install"], cwd=source_dir):
        return False

    return True


def build_freetype_linux(source_dir: Path, install_dir: Path) -> bool:
    """Build FreeType on Linux using configure."""
    print("Building FreeType for Linux...")

    # Configure
    configure_args = [
        "./configure",
        f"--prefix={install_dir}",
        "--without-zlib",
        "--without-bzip2",
        "--without-png",
        "--without-harfbuzz",
        "--without-brotli",
        "--enable-static",
        "--disable-shared",
    ]

    if not run_command(configure_args, cwd=source_dir):
        return False

    # Build (use all available cores)
    import multiprocessing
    cores = multiprocessing.cpu_count()
    if not run_command(["make", f"-j{cores}"], cwd=source_dir):
        return False

    # Install
    if not run_command(["make", "install"], cwd=source_dir):
        return False

    return True


def organize_installation(install_dir: Path, dest_dir: Path) -> bool:
    """Organize the installation into the xs external structure."""
    print("Organizing installation...")
    
    try:
        # Create destination directories
        inc_dest = dest_dir / "inc"
        lib_dest = dest_dir / "lib"
        
        inc_dest.mkdir(parents=True, exist_ok=True)
        lib_dest.mkdir(parents=True, exist_ok=True)
        
        # Copy headers (flatten the freetype2 directory structure)
        freetype_inc = install_dir / "include" / "freetype2"
        if freetype_inc.exists():
            # Copy all headers from freetype2/freetype/ to inc/freetype/
            src_headers = freetype_inc / "freetype"
            if src_headers.exists():
                dst_headers = inc_dest / "freetype"
                if dst_headers.exists():
                    shutil.rmtree(dst_headers)
                shutil.copytree(src_headers, dst_headers)
            
            # Also copy ft2build.h to inc/
            ft2build = freetype_inc / "ft2build.h"
            if ft2build.exists():
                shutil.copy2(ft2build, inc_dest / "ft2build.h")
        
        # Copy libraries based on platform
        plat = platform.system()
        if plat == "Windows":
            lib_src = install_dir / "lib"
            lib_platform_dest = lib_dest / "win"
            lib_platform_dest.mkdir(parents=True, exist_ok=True)
            
            if lib_src.exists():
                # Copy static/import libraries
                for lib_file in lib_src.glob("*.lib"):
                    shutil.copy2(lib_file, lib_platform_dest / lib_file.name)
                for lib_file in lib_src.glob("*.a"):
                    shutil.copy2(lib_file, lib_platform_dest / lib_file.name)
                # Copy runtime DLL if present (dynamic build case)
                for dll_file in lib_src.glob("*.dll"):
                    shutil.copy2(dll_file, lib_platform_dest / dll_file.name)
        
        elif plat == "Darwin":
            lib_src = install_dir / "lib"
            lib_platform_dest = lib_dest / "mac"
            lib_platform_dest.mkdir(parents=True, exist_ok=True)

            # Copy all .a files (static libraries)
            if lib_src.exists():
                for lib_file in lib_src.glob("*.a"):
                    shutil.copy2(lib_file, lib_platform_dest / lib_file.name)

            # Also build for iOS
            print("Building for iOS...")
            ios_install_dir = install_dir.parent / "install_ios"
            ios_source_dir = install_dir.parent.parent / FREETYPE_DIR

            if ios_source_dir.exists():
                if build_freetype_ios(ios_source_dir, ios_install_dir):
                    # Copy iOS libraries
                    ios_lib_src = ios_install_dir / "lib"
                    ios_lib_dest = lib_dest / "ios"
                    ios_lib_dest.mkdir(parents=True, exist_ok=True)

                    if ios_lib_src.exists():
                        for lib_file in ios_lib_src.glob("*.a"):
                            shutil.copy2(lib_file, ios_lib_dest / lib_file.name)

                    # Clean up iOS install directory
                    shutil.rmtree(ios_install_dir, ignore_errors=True)
                else:
                    print("Warning: Failed to build FreeType for iOS, continuing...")

        elif plat == "Linux":
            lib_src = install_dir / "lib"
            lib_platform_dest = lib_dest / "linux"
            lib_platform_dest.mkdir(parents=True, exist_ok=True)

            # Copy all .a files (static libraries)
            if lib_src.exists():
                for lib_file in lib_src.glob("*.a"):
                    shutil.copy2(lib_file, lib_platform_dest / lib_file.name)
        
        # Copy license
        license_src = install_dir / "share" / "licenses" / "freetype2" / "LICENSE.TXT"
        if not license_src.exists():
            # Try alternative location
            license_src = install_dir / ".." / "LICENSE.TXT"
        
        if license_src.exists():
            shutil.copy2(license_src, dest_dir / "LICENSE.TXT")
        
        print(f"Installation organized in {dest_dir}")
        return True
    
    except Exception as e:
        print(f"Error organizing installation: {e}")
        return False


def main():
    """Main build process."""
    print("=" * 60)
    print("Building FreeType for xs game engine")
    print(f"Platform: {platform.system()}")
    print("=" * 60)

    plat = platform.system()
    if plat not in ["Windows", "Darwin", "Linux"]:
        print(f"Error: FreeType build not supported on {plat}")
        return 1

    repo_root = get_repo_root()
    temp_dir = repo_root / "external" / ".temp_freetype_build"
    install_dir = temp_dir / "install"
    dest_dir = repo_root / "external" / "freetype"

    try:
        # Create temp directory
        temp_dir.mkdir(parents=True, exist_ok=True)

        # Download FreeType
        if not download_freetype(temp_dir):
            print("Failed to download FreeType")
            return 1

        # Extract
        if not extract_archive(temp_dir):
            print("Failed to extract FreeType")
            return 1

        source_dir = temp_dir / FREETYPE_DIR

        # Build based on platform
        if plat == "Windows":
            if not build_freetype_windows(source_dir, install_dir):
                print("Failed to build FreeType on Windows")
                return 1

        elif plat == "Darwin":
            if not build_freetype_macos(source_dir, install_dir):
                print("Failed to build FreeType on macOS")
                return 1

        elif plat == "Linux":
            if not build_freetype_linux(source_dir, install_dir):
                print("Failed to build FreeType on Linux")
                return 1
        
        # Organize installation
        if not organize_installation(install_dir, dest_dir):
            print("Failed to organize installation")
            return 1
        
        print("=" * 60)
        print("FreeType build completed successfully!")
        print(f"Installed to: {dest_dir}")
        print("=" * 60)
        
        # Clean up temp directory (keep if KEEP_TEMP set)
        if temp_dir.exists() and not os.getenv("KEEP_TEMP"):
            shutil.rmtree(temp_dir)
        
        return 0
    
    except Exception as e:
        print(f"Error during build: {e}")
        return 1
    finally:
        # Clean up on error
        if temp_dir.exists() and not os.getenv("KEEP_TEMP"):
            try:
                shutil.rmtree(temp_dir)
            except:
                pass


if __name__ == "__main__":
    sys.exit(main())
