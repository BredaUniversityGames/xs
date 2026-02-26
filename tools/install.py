#!/usr/bin/env python3
"""
Install script for xs game engine
Builds the specified configuration and installs the engine with resources to user directories
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path


def get_project_root():
    """Get the project root directory"""
    script_dir = Path(__file__).parent
    return script_dir.parent.resolve()


def get_platform_name():
    """Detect the current platform"""
    system = platform.system()
    if system == "Windows":
        return "windows"
    elif system == "Linux":
        return "linux"
    elif system == "Darwin":
        return "macos"
    else:
        raise RuntimeError(f"Unsupported platform: {system}")


def get_default_install_dir(platform_name):
    """Get the default installation directory for the platform"""
    if platform_name == "windows":
        # User-level install in AppData\Local
        base = Path(os.environ.get("LOCALAPPDATA", "~/AppData/Local")).expanduser()
        return base / "xs"
    
    elif platform_name == "linux":
        # User-level install in home directory
        return Path.home() / ".local" / "share" / "xs"
    
    elif platform_name == "macos":
        return Path.home() / "Applications" / "xs"
    
    return None


def get_bin_dir(platform_name):
    """Get the binary installation directory"""
    if platform_name == "linux":
        return Path.home() / ".local" / "bin"
    return None  # Windows and macOS keep binaries with resources


def add_to_path_windows(install_dir):
    """Add directory to Windows user PATH"""
    import winreg
    
    install_dir_str = str(install_dir)
    
    try:
        # Open the user environment variables registry key
        key = winreg.OpenKey(
            winreg.HKEY_CURRENT_USER,
            r"Environment",
            0,
            winreg.KEY_READ | winreg.KEY_WRITE
        )
        
        try:
            # Get current PATH
            current_path, _ = winreg.QueryValueEx(key, "Path")
        except FileNotFoundError:
            current_path = ""
        
        # Check if already in PATH
        path_dirs = [p.strip() for p in current_path.split(";") if p.strip()]
        
        if install_dir_str not in path_dirs:
            # Add to PATH
            if current_path and not current_path.endswith(";"):
                new_path = current_path + ";" + install_dir_str
            else:
                new_path = current_path + install_dir_str
            
            # Set the new PATH
            winreg.SetValueEx(key, "Path", 0, winreg.REG_EXPAND_SZ, new_path)
            
            # Notify the system of environment change
            import ctypes
            HWND_BROADCAST = 0xFFFF
            WM_SETTINGCHANGE = 0x1A
            SMTO_ABORTIFHUNG = 0x0002
            result = ctypes.c_long()
            SendMessageTimeoutW = ctypes.windll.user32.SendMessageTimeoutW
            SendMessageTimeoutW(
                HWND_BROADCAST,
                WM_SETTINGCHANGE,
                0,
                "Environment",
                SMTO_ABORTIFHUNG,
                5000,
                ctypes.byref(result)
            )
        
        winreg.CloseKey(key)
    except Exception as e:
        print(f"  Warning: Could not automatically add to PATH: {e}")
        print(f"  Please manually add this directory to your PATH:")
        print(f"  {install_dir}")


def add_to_path_unix(bin_dir):
    """Add directory to Unix PATH in shell config"""
    bin_dir_str = str(bin_dir)
    
    # Check if already in current PATH
    if bin_dir_str in os.environ.get("PATH", "").split(":"):
        return
    
    # Determine which shell config file to use
    shell_configs = []
    home = Path.home()
    
    # Check for common shell config files
    if (home / ".bashrc").exists():
        shell_configs.append(home / ".bashrc")
    if (home / ".zshrc").exists():
        shell_configs.append(home / ".zshrc")
    
    # If none exist, create .bashrc
    if not shell_configs:
        shell_configs.append(home / ".bashrc")
    
    export_line = f'export PATH="$PATH:{bin_dir_str}"\n'
    
    for config_file in shell_configs:
        try:
            # Check if already in file
            if config_file.exists():
                content = config_file.read_text()
                if bin_dir_str in content:
                    continue
            
            # Add to file
            with open(config_file, "a") as f:
                f.write(f"\n# Added by xs install script\n")
                f.write(export_line)
            
            print(f"  Updated {config_file}")
        except Exception as e:
            print(f"  Warning: Could not update {config_file}: {e}")


def build_windows(config="Develop", clean=False):
    """Build the Windows version using MSBuild"""
    project_root = get_project_root()
    solution_file = project_root / "xs.sln"
    
    if not solution_file.exists():
        raise FileNotFoundError(f"Solution file not found: {solution_file}")
    
    print(f"Building Windows {config} configuration...")
    
    cmd = [
        "msbuild",
        str(solution_file),
        f"/p:Configuration={config}",
        "/p:Platform=PC",
        "/m"
    ]
    
    if clean:
        cmd.append("/t:Clean,Build")
    
    result = subprocess.run(cmd, cwd=project_root)
    if result.returncode != 0:
        raise RuntimeError(f"Build failed with exit code {result.returncode}")
    
    return project_root / "build" / "executable" / "x64" / config


def build_linux(config="Develop", clean=False):
    """Build the Linux version using CMake"""
    project_root = get_project_root()
    build_dir = project_root / "build" / "linux" / config.lower()
    
    # Create build directory
    build_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Configuring Linux {config} build...")
    
    # Configure CMake
    cmake_args = [
        "cmake",
        str(project_root),
        f"-DCMAKE_BUILD_TYPE={config}",
    ]
    
    result = subprocess.run(cmake_args, cwd=build_dir)
    if result.returncode != 0:
        raise RuntimeError(f"CMake configuration failed with exit code {result.returncode}")
    
    print(f"Building Linux {config} configuration...")
    
    # Build
    make_args = ["cmake", "--build", ".", "--config", config, "-j"]
    
    if clean:
        subprocess.run(["cmake", "--build", ".", "--target", "clean"], cwd=build_dir)
    
    result = subprocess.run(make_args, cwd=build_dir)
    if result.returncode != 0:
        raise RuntimeError(f"Build failed with exit code {result.returncode}")
    
    return build_dir


def build_macos(config="Develop", clean=False):
    """Build the macOS version using Xcode"""
    project_root = get_project_root()
    xcode_project = project_root / "xs.xcodeproj"
    
    if not xcode_project.exists():
        raise FileNotFoundError(f"Xcode project not found: {xcode_project}")
    
    print(f"Building macOS {config} configuration...")
    
    cmd = [
        "xcodebuild",
        "-project", str(xcode_project),
        "-configuration", config,
        "build"
    ]
    
    if clean:
        subprocess.run(["xcodebuild", "-project", str(xcode_project), "-configuration", config, "clean"], 
                      cwd=project_root)
    
    result = subprocess.run(cmd, cwd=project_root)
    if result.returncode != 0:
        raise RuntimeError(f"Build failed with exit code {result.returncode}")
    
    return project_root / "build" / config


def install_to_system(build_dir, install_dir, platform_name):
    """Install the built engine to the user directory"""
    project_root = get_project_root()
    
    print(f"\nInstalling xs engine to: {install_dir}")
    
    # Create installation directory
    install_dir.mkdir(parents=True, exist_ok=True)
    
    if platform_name == "windows":
        # Copy executable and DLLs
        print("Installing executable and dependencies...")
        for item in build_dir.glob("*.exe"):
            shutil.copy2(item, install_dir)
            print(f"  Installed {item.name}")
        
        for item in build_dir.glob("*.dll"):
            shutil.copy2(item, install_dir)
            print(f"  Installed {item.name}")
        
        # Copy resources
        resources_dir = project_root / "resources"
        if resources_dir.exists():
            print("Installing resources...")
            resource_folders = ["fonts", "images", "modules", "shaders"]
            
            for folder in resource_folders:
                src = resources_dir / folder
                if src.exists():
                    dst = install_dir / "resources" / folder
                    if dst.exists():
                        shutil.rmtree(dst)
                    shutil.copytree(src, dst)
                    print(f"  Installed resources/{folder}/")
            
            # Copy icon
            icon_file = resources_dir / "xs.ico"
            if icon_file.exists():
                (install_dir / "resources").mkdir(exist_ok=True)
                shutil.copy2(icon_file, install_dir / "resources")
                print(f"  Installed resources/xs.ico")
        
        # Add to PATH
        print(f"\nAdding to user PATH...")
        add_to_path_windows(install_dir)
        print(f"  Added {install_dir} to PATH")
        print(f"\n  Restart your terminal to use 'xs' command")
        print(f"  Or run xs directly from:")
        print(f"  {install_dir / 'xs.exe'}")
    
    elif platform_name == "linux":
        # Install executable to bin directory
        bin_dir = get_bin_dir(platform_name)
        bin_dir.mkdir(parents=True, exist_ok=True)
        
        exe_src = build_dir / "xs"
        exe_dst = bin_dir / "xs"
        
        print(f"Installing executable to {bin_dir}...")
        shutil.copy2(exe_src, exe_dst)
        exe_dst.chmod(0o755)  # Make executable
        print(f"  Installed xs")
        
        # Install resources to share directory
        resources_dir = project_root / "resources"
        if resources_dir.exists():
            print(f"Installing resources to {install_dir}...")
            resource_folders = ["fonts", "images", "modules", "shaders"]
            
            for folder in resource_folders:
                src = resources_dir / folder
                if src.exists():
                    dst = install_dir / folder
                    if dst.exists():
                        shutil.rmtree(dst)
                    shutil.copytree(src, dst)
                    print(f"  Installed {folder}/")
        
        # Install desktop file if exists
        desktop_file = project_root / "platforms" / "linux" / "xs.desktop"
        if desktop_file.exists():
            desktop_dir = Path.home() / ".local" / "share" / "applications"
            desktop_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(desktop_file, desktop_dir / "xs.desktop")
            print(f"  Installed desktop launcher")
        
        # Add to PATH
        print(f"\nAdding to PATH...")
        add_to_path_unix(bin_dir)
        print(f"  Added {bin_dir} to PATH in shell config")
        print(f"  Restart your terminal or run: source ~/.bashrc")
    
    elif platform_name == "macos":
        # Install app bundle or executable
        exe_src = build_dir / "xs"
        
        print(f"Installing to {install_dir}...")
        install_dir.mkdir(parents=True, exist_ok=True)
        
        shutil.copy2(exe_src, install_dir / "xs")
        (install_dir / "xs").chmod(0o755)
        print(f"  Installed xs")
        
        # Install resources
        resources_dir = project_root / "resources"
        if resources_dir.exists():
            print("Installing resources...")
            resource_folders = ["fonts", "images", "modules", "shaders"]
            
            for folder in resource_folders:
                src = resources_dir / folder
                if src.exists():
                    dst = install_dir / "resources" / folder
                    if dst.exists():
                        shutil.rmtree(dst)
                    shutil.copytree(src, dst)
                    print(f"  Installed resources/{folder}/")
        
        print(f"\n  Run xs from: {install_dir / 'xs'}")


def install_platform(platform_name, config="Develop", clean=False, install_dir=None):
    """Build and install for the specified platform"""
    print(f"\n{'='*60}")
    print(f"Installing xs engine for {platform_name.upper()} ({config})")
    print(f"{'='*60}\n")
    
    # Build for the platform
    if platform_name == "windows":
        build_dir = build_windows(config, clean)
    elif platform_name == "linux":
        build_dir = build_linux(config, clean)
    elif platform_name == "macos":
        build_dir = build_macos(config, clean)
    else:
        raise ValueError(f"Unknown platform: {platform_name}")
    
    print(f"\nBuild completed successfully!")
    print(f"Build directory: {build_dir}\n")
    
    # Determine installation directory
    if install_dir is None:
        install_dir = get_default_install_dir(platform_name)
    else:
        install_dir = Path(install_dir).expanduser().resolve()
    
    # Install to system
    install_to_system(build_dir, install_dir, platform_name)
    
    print(f"\n{'='*60}")
    print(f"Installation completed!")
    print(f"xs engine is installed in: {install_dir}")
    print(f"{'='*60}\n")
    
    return install_dir


def main():
    parser = argparse.ArgumentParser(
        description="Build and install xs game engine to user directories",
        epilog="Examples:\n"
               "  python install.py                                    # Install to default user directory\n"
               "  python install.py --prefix C:\\Tools\\xs              # Install to custom location\n"
               "  python install.py --config Release --clean           # Clean Release build and install\n",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--platform",
        choices=["windows", "linux", "macos", "auto"],
        default="auto",
        help="Target platform (default: auto-detect)"
    )
    parser.add_argument(
        "--config",
        choices=["Debug", "Develop", "Release"],
        default="Develop",
        help="Build configuration (default: Develop)"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean before building"
    )
    parser.add_argument(
        "--prefix",
        type=str,
        help="Custom installation directory (overrides default)"
    )
    
    args = parser.parse_args()
    
    # Auto-detect platform if requested
    if args.platform == "auto":
        target_platform = get_platform_name()
        print(f"Auto-detected platform: {target_platform}")
    else:
        target_platform = args.platform
    
    try:
        install_dir = install_platform(
            target_platform, 
            args.config, 
            args.clean, 
            args.prefix
        )
        return 0
    except Exception as e:
        print(f"\nError: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
