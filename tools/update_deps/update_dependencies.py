#!/usr/bin/env python3
"""
XS Dependency Updater
Interactive script to update external dependencies for the xs game engine.
"""

import os
import sys
import shutil
import subprocess
from pathlib import Path
from typing import Dict, List, Optional

try:
    from prompt_toolkit import prompt
    from prompt_toolkit.completion import WordCompleter
    from prompt_toolkit.styles import Style
    PROMPT_TOOLKIT_AVAILABLE = True
except ImportError:
    PROMPT_TOOLKIT_AVAILABLE = False
    print("Note: Install 'prompt_toolkit' for better UI: pip install prompt_toolkit")


# Color codes for terminal output
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


# Dependency definitions
DEPENDENCIES = {
    'imgui': {
        'name': 'Dear ImGui',
        'type': 'opensource',
        'repo': 'https://github.com/ocornut/imgui.git',
        'branch': 'master',
        'description': 'Immediate mode GUI library',
    },
    'fmt': {
        'name': 'fmt',
        'type': 'opensource',
        'repo': 'https://github.com/fmtlib/fmt.git',
        'branch': 'master',
        'description': 'Modern formatting library',
    },
    'glm': {
        'name': 'GLM',
        'type': 'opensource',
        'repo': 'https://github.com/g-truc/glm.git',
        'branch': 'master',
        'description': 'OpenGL Mathematics library',
    },
    'json': {
        'name': 'nlohmann/json',
        'type': 'opensource',
        'repo': 'https://github.com/nlohmann/json.git',
        'branch': 'develop',
        'description': 'JSON for Modern C++',
    },
    'stb': {
        'name': 'stb',
        'type': 'opensource',
        'repo': 'https://github.com/nothings/stb.git',
        'branch': 'master',
        'description': 'Single-file public domain libraries',
    },
    'wren': {
        'name': 'Wren',
        'type': 'opensource',
        'repo': 'https://github.com/wren-lang/wren.git',
        'branch': 'main',
        'description': 'Scripting language',
    },
    'nanosvg': {
        'name': 'NanoSVG',
        'type': 'opensource',
        'repo': 'https://github.com/memononen/nanosvg.git',
        'branch': 'master',
        'description': 'Simple SVG parser',
    },
    'miniz': {
        'name': 'miniz',
        'type': 'opensource',
        'repo': 'https://github.com/richgel999/miniz.git',
        'branch': 'master',
        'description': 'Compression library',
    },
    'dialogs': {
        'name': 'Portable File Dialogs',
        'type': 'opensource',
        'repo': 'https://github.com/samhocevar/portable-file-dialogs.git',
        'branch': 'main',
        'description': 'Cross-platform file dialogs',
    },
    'glad': {
        'name': 'GLAD',
        'type': 'opensource',
        'repo': 'https://github.com/Dav1dde/glad.git',
        'branch': 'master',
        'description': 'OpenGL loader',
    },
    'fmod': {
        'name': 'FMOD',
        'type': 'local',
        'platforms': ['pc', 'nx', 'prospero', 'apple'],
        'description': 'Audio engine (requires local installation)',
        'install_notes': 'Install FMOD Studio API from https://www.fmod.com/download',
    },
    'steam': {
        'name': 'Steam API',
        'type': 'local',
        'platforms': ['pc'],
        'description': 'Steamworks SDK (requires local installation)',
        'install_notes': 'Install Steamworks SDK from https://partner.steamgames.com/',
    },
}


def print_header(text: str):
    """Print a colored header."""
    print(f"\n{Colors.BOLD}{Colors.HEADER}{'='*60}{Colors.ENDC}")
    print(f"{Colors.BOLD}{Colors.HEADER}{text:^60}{Colors.ENDC}")
    print(f"{Colors.BOLD}{Colors.HEADER}{'='*60}{Colors.ENDC}\n")


def print_success(text: str):
    """Print a success message."""
    print(f"{Colors.OKGREEN}✓ {text}{Colors.ENDC}")


def print_error(text: str):
    """Print an error message."""
    print(f"{Colors.FAIL}✗ {text}{Colors.ENDC}")


def print_warning(text: str):
    """Print a warning message."""
    print(f"{Colors.WARNING}⚠ {text}{Colors.ENDC}")


def print_info(text: str):
    """Print an info message."""
    print(f"{Colors.OKCYAN}ℹ {text}{Colors.ENDC}")


def get_repo_root() -> Path:
    """Get the repository root directory."""
    script_dir = Path(__file__).parent
    return script_dir.parent


def run_command(cmd: List[str], cwd: Optional[Path] = None) -> tuple[bool, str]:
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


def update_opensource_dependency(dep_key: str, dep_info: Dict) -> bool:
    """Update an open-source dependency from its git repository."""
    repo_root = get_repo_root()
    dep_path = repo_root / 'external' / dep_key

    print_info(f"Updating {dep_info['name']}...")
    print(f"  Repository: {dep_info['repo']}")
    print(f"  Branch: {dep_info['branch']}")

    # Create temporary directory for cloning
    temp_dir = repo_root / 'external' / f'.temp_{dep_key}'

    try:
        # Clone the repository
        print_info("Cloning repository...")
        success, output = run_command([
            'git', 'clone',
            '--depth', '1',
            '--branch', dep_info['branch'],
            dep_info['repo'],
            str(temp_dir)
        ])

        if not success:
            print_error(f"Failed to clone repository: {output}")
            return False

        # Remove .git directory from clone
        git_dir = temp_dir / '.git'
        if git_dir.exists():
            shutil.rmtree(git_dir)

        # Backup existing dependency
        backup_path = None
        if dep_path.exists():
            backup_path = repo_root / 'external' / f'.backup_{dep_key}'
            print_info("Backing up existing version...")
            if backup_path.exists():
                shutil.rmtree(backup_path)
            shutil.move(str(dep_path), str(backup_path))

        # Move new version into place
        print_info("Installing new version...")
        shutil.move(str(temp_dir), str(dep_path))

        # Remove backup if successful
        if backup_path and backup_path.exists():
            shutil.rmtree(backup_path)

        print_success(f"Successfully updated {dep_info['name']}")
        return True

    except Exception as e:
        print_error(f"Error updating {dep_info['name']}: {e}")

        # Restore backup if it exists
        backup_path = repo_root / 'external' / f'.backup_{dep_key}'
        if backup_path and backup_path.exists():
            if dep_path.exists():
                shutil.rmtree(dep_path)
            shutil.move(str(backup_path), str(dep_path))
            print_info("Restored previous version")

        # Clean up temp directory
        if temp_dir.exists():
            shutil.rmtree(temp_dir)

        return False


def show_local_dependency_info(dep_key: str, dep_info: Dict, platform: Optional[str] = None):
    """Show information about a local dependency."""
    print_info(f"{dep_info['name']}")
    print(f"  Type: Local installation required")
    print(f"  Description: {dep_info['description']}")

    if platform:
        print(f"  Platform: {platform}")
    else:
        print(f"  Platforms: {', '.join(dep_info['platforms'])}")

    print(f"\n  {Colors.BOLD}Installation Instructions:{Colors.ENDC}")
    print(f"  {dep_info['install_notes']}")

    repo_root = get_repo_root()
    dep_path = repo_root / 'external' / dep_key

    if platform:
        platform_path = dep_path / platform
        if platform_path.exists():
            print_success(f"Found at: {platform_path}")
        else:
            print_warning(f"Not found at: {platform_path}")
    else:
        if dep_path.exists():
            print_success(f"Found at: {dep_path}")
            # Show which platforms are available
            available_platforms = [p for p in dep_info['platforms']
                                 if (dep_path / p).exists()]
            if available_platforms:
                print(f"  Available platforms: {', '.join(available_platforms)}")
        else:
            print_warning(f"Not found at: {dep_path}")


def list_dependencies():
    """List all dependencies."""
    print_header("XS Dependencies")

    print(f"{Colors.BOLD}Open-Source Libraries:{Colors.ENDC}")
    for key, info in DEPENDENCIES.items():
        if info['type'] == 'opensource':
            print(f"  {Colors.OKCYAN}{key:15}{Colors.ENDC} - {info['name']:25} - {info['description']}")

    print(f"\n{Colors.BOLD}Local Dependencies:{Colors.ENDC}")
    for key, info in DEPENDENCIES.items():
        if info['type'] == 'local':
            platforms = ', '.join(info['platforms'])
            print(f"  {Colors.WARNING}{key:15}{Colors.ENDC} - {info['name']:25} - {platforms}")


def interactive_menu():
    """Show interactive menu for selecting dependencies."""
    print_header("XS Dependency Updater")

    # Check prerequisites
    if not check_git_available():
        print_error("Git is not available. Please install git first.")
        return

    while True:
        print(f"\n{Colors.BOLD}What would you like to do?{Colors.ENDC}")
        print("  1. List all dependencies")
        print("  2. Update an open-source dependency")
        print("  3. Show local dependency info")
        print("  4. Update all open-source dependencies")
        print("  5. Exit")

        if PROMPT_TOOLKIT_AVAILABLE:
            completer = WordCompleter(['1', '2', '3', '4', '5'], ignore_case=True)
            choice = prompt('\nChoice: ', completer=completer).strip()
        else:
            choice = input('\nChoice: ').strip()

        if choice == '1':
            list_dependencies()

        elif choice == '2':
            # Show available open-source dependencies
            opensource_deps = {k: v for k, v in DEPENDENCIES.items()
                             if v['type'] == 'opensource'}

            print(f"\n{Colors.BOLD}Available open-source dependencies:{Colors.ENDC}")
            for i, (key, info) in enumerate(opensource_deps.items(), 1):
                print(f"  {i}. {key} - {info['name']}")

            if PROMPT_TOOLKIT_AVAILABLE:
                dep_keys = list(opensource_deps.keys())
                completer = WordCompleter(dep_keys, ignore_case=True)
                dep_choice = prompt('\nEnter dependency name: ', completer=completer).strip()
            else:
                dep_choice = input('\nEnter dependency name: ').strip()

            if dep_choice in opensource_deps:
                update_opensource_dependency(dep_choice, opensource_deps[dep_choice])
            else:
                print_error(f"Unknown dependency: {dep_choice}")

        elif choice == '3':
            # Show available local dependencies
            local_deps = {k: v for k, v in DEPENDENCIES.items()
                        if v['type'] == 'local'}

            print(f"\n{Colors.BOLD}Available local dependencies:{Colors.ENDC}")
            for i, (key, info) in enumerate(local_deps.items(), 1):
                print(f"  {i}. {key} - {info['name']}")

            if PROMPT_TOOLKIT_AVAILABLE:
                dep_keys = list(local_deps.keys())
                completer = WordCompleter(dep_keys, ignore_case=True)
                dep_choice = prompt('\nEnter dependency name: ', completer=completer).strip()
            else:
                dep_choice = input('\nEnter dependency name: ').strip()

            if dep_choice in local_deps:
                dep_info = local_deps[dep_choice]

                # Ask for platform if multiple available
                if len(dep_info['platforms']) > 1:
                    print(f"\n{Colors.BOLD}Available platforms:{Colors.ENDC}")
                    for i, platform in enumerate(dep_info['platforms'], 1):
                        print(f"  {i}. {platform}")

                    if PROMPT_TOOLKIT_AVAILABLE:
                        completer = WordCompleter(dep_info['platforms'], ignore_case=True)
                        platform = prompt('\nEnter platform (or press Enter for all): ',
                                        completer=completer).strip()
                    else:
                        platform = input('\nEnter platform (or press Enter for all): ').strip()

                    if platform and platform in dep_info['platforms']:
                        show_local_dependency_info(dep_choice, dep_info, platform)
                    elif not platform:
                        show_local_dependency_info(dep_choice, dep_info)
                    else:
                        print_error(f"Unknown platform: {platform}")
                else:
                    show_local_dependency_info(dep_choice, dep_info)
            else:
                print_error(f"Unknown dependency: {dep_choice}")

        elif choice == '4':
            print_warning("This will update all open-source dependencies.")
            confirm = input("Continue? (y/N): ").strip().lower()

            if confirm == 'y':
                opensource_deps = {k: v for k, v in DEPENDENCIES.items()
                                 if v['type'] == 'opensource'}

                success_count = 0
                fail_count = 0

                for key, info in opensource_deps.items():
                    print()
                    if update_opensource_dependency(key, info):
                        success_count += 1
                    else:
                        fail_count += 1

                print_header("Update Summary")
                print_success(f"Successfully updated: {success_count}")
                if fail_count > 0:
                    print_error(f"Failed to update: {fail_count}")
            else:
                print_info("Cancelled")

        elif choice == '5':
            print_info("Goodbye!")
            break

        else:
            print_error("Invalid choice. Please try again.")


def main():
    """Main entry point."""
    try:
        interactive_menu()
    except KeyboardInterrupt:
        print(f"\n\n{Colors.WARNING}Interrupted by user{Colors.ENDC}")
        sys.exit(0)
    except Exception as e:
        print_error(f"Unexpected error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
