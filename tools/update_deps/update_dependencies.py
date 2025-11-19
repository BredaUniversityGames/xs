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
        'description': 'Audio engine (auto-copy from Program Files on Windows)',
        'install_notes': 'Install FMOD Studio API from https://www.fmod.com/download',
        'update_function': 'copy_fmod_from_program_files',  # Custom update handler
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


def check_dependency_installed(dep_key: str, dep_info: Dict) -> bool:
    """Check if a dependency is installed."""
    repo_root = get_repo_root()
    dep_path = repo_root / 'external' / dep_key

    if dep_info['type'] == 'opensource':
        # For open-source, just check if directory exists and has content
        return dep_path.exists() and any(dep_path.iterdir())
    elif dep_info['type'] == 'local':
        # For local deps, check if the platform-specific directory exists
        platform_key = get_platform_key()
        if platform_key in dep_info['platforms']:
            platform_path = dep_path / platform_key
            return platform_path.exists() and any(platform_path.iterdir())
        return False
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
    repo_root = get_repo_root()
    dep_path = repo_root / 'external' / dep_key
    temp_dir = repo_root / 'external' / f'.temp_{dep_key}'

    try:
        # Clone the repository
        if progress_callback:
            progress_callback("Cloning repository...")

        success, output = run_command([
            'git', 'clone',
            '--depth', '1',
            '--branch', dep_info['branch'],
            dep_info['repo'],
            str(temp_dir)
        ])

        if not success:
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
            backup_path = repo_root / 'external' / f'.backup_{dep_key}'
            if backup_path.exists():
                shutil.rmtree(backup_path)
            shutil.move(str(dep_path), str(backup_path))

        # Copy filtered files/directories
        if progress_callback:
            progress_callback("Copying files...")

        # Create destination directory
        dep_path.mkdir(parents=True, exist_ok=True)

        # Check if include_paths is specified
        if 'include_paths' in dep_info and dep_info['include_paths']:
            # Copy only specified paths
            copy_filtered_paths(temp_dir, dep_path, dep_info['include_paths'])
        else:
            # No filter - copy everything (old behavior)
            shutil.copytree(temp_dir, dep_path, dirs_exist_ok=True)

        # Clean up temp directory
        if temp_dir.exists():
            shutil.rmtree(temp_dir)

        # Remove backup if successful
        if backup_path and backup_path.exists():
            shutil.rmtree(backup_path)

        if progress_callback:
            progress_callback("Complete")

        return True, "Successfully updated"

    except Exception as e:
        # Restore backup if it exists
        backup_path = repo_root / 'external' / f'.backup_{dep_key}'
        if backup_path and backup_path.exists():
            if dep_path.exists():
                shutil.rmtree(dep_path)
            shutil.move(str(backup_path), str(dep_path))

        # Clean up temp directory
        if temp_dir.exists():
            shutil.rmtree(temp_dir)

        return False, str(e)


def copy_fmod_from_program_files(progress_callback=None) -> Tuple[bool, str]:
    """Copy FMOD files from Program Files installation to external/fmod."""
    repo_root = get_repo_root()
    plat = platform.system()

    if plat == "Windows":
        fmod_base = Path(r"C:\Program Files (x86)\FMOD SoundSystem")
        fmod_windows = fmod_base / "FMOD Studio API Windows"

        if not fmod_windows.exists():
            return False, f"FMOD not found at {fmod_windows}"

        try:
            if progress_callback:
                progress_callback("Copying headers...")

            # Source paths
            core_inc = fmod_windows / "api" / "core" / "inc"
            core_lib = fmod_windows / "api" / "core" / "lib" / "x64"
            studio_inc = fmod_windows / "api" / "studio" / "inc"
            studio_lib = fmod_windows / "api" / "studio" / "lib" / "x64"

            # Destination paths
            fmod_dest = repo_root / "external" / "fmod" / "pc"
            inc_dest = fmod_dest / "inc"
            lib_dest = fmod_dest / "lib"

            # Create destination directories
            inc_dest.mkdir(parents=True, exist_ok=True)
            lib_dest.mkdir(parents=True, exist_ok=True)

            # Copy include files
            if core_inc.exists():
                shutil.copytree(core_inc, inc_dest, dirs_exist_ok=True)
            if studio_inc.exists():
                shutil.copytree(studio_inc, inc_dest, dirs_exist_ok=True)

            if progress_callback:
                progress_callback("Copying libraries...")

            # Copy lib files
            if core_lib.exists():
                shutil.copytree(core_lib, lib_dest, dirs_exist_ok=True)
            if studio_lib.exists():
                shutil.copytree(studio_lib, lib_dest, dirs_exist_ok=True)

            if progress_callback:
                progress_callback("Complete")

            return True, "Successfully copied FMOD"

        except Exception as e:
            return False, str(e)

    elif plat == "Darwin":
        return False, "FMOD copy on macOS not implemented. Please copy manually to external/fmod/apple/"

    elif plat == "Linux":
        return False, "FMOD copy on Linux not implemented. Please copy manually to external/fmod/pc/"

    else:
        return False, f"Unsupported platform: {plat}"


class DependencyUpdaterApp(App):
    """Textual app for managing XS dependencies."""

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
        table.add_column("Name", width=25, key="name")
        table.add_column("Type", width=15, key="type")
        table.add_column("Status", width=15, key="status")
        table.add_column("Description", width=40, key="description")
        table.add_column("Progress", width=30, key="progress")

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

            # Determine type string
            type_str = dep_info['type'].capitalize()
            if dep_info['type'] == 'local':
                type_str += f" ({platform_key})"

            # Status - different messaging for opensource vs local
            if dep_info['type'] == 'opensource':
                if installed:
                    status = "[green]✓ Installed[/green]"
                else:
                    status = "[cyan]○ Available[/cyan]"
            else:  # local dependency
                if installed:
                    status = "[green]✓ Installed[/green]"
                else:
                    status = "[red]✗ Not Found[/red]"

            # Checkbox state
            checkbox = "☑" if dep_key in self.selected_deps else "☐"

            # Add row
            table.add_row(
                checkbox,
                dep_key,
                dep_info['name'],
                type_str,
                status,
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
        for dep_key in dep_keys:
            dep_info = DEPENDENCIES[dep_key]

            # Update progress in table
            self.call_from_thread(self.update_progress, dep_key, "⏳ Starting...")

            def progress_callback(msg):
                self.call_from_thread(self.update_progress, dep_key, f"⏳ {msg}")

            # Check if dependency has a custom update function
            if 'update_function' in dep_info:
                # Call custom update function by name
                func_name = dep_info['update_function']
                if func_name == 'copy_fmod_from_program_files':
                    success, message = copy_fmod_from_program_files(progress_callback)
                else:
                    success, message = False, f"Unknown update function: {func_name}"
            else:
                # Default opensource update
                success, message = update_opensource_dependency(dep_key, dep_info, progress_callback)

            if success:
                self.call_from_thread(self.update_progress, dep_key, "[green]✓ Complete[/green]")
                self.call_from_thread(self.notify, f"✓ {dep_key} updated successfully", severity="information")
            else:
                self.call_from_thread(self.update_progress, dep_key, f"[red]✗ Failed: {message}[/red]")
                self.call_from_thread(self.notify, f"✗ {dep_key} failed: {message}", severity="error")

        # Done
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

        # Update the progress column (now index 6, after swapping Description and Progress)
        table.update_cell_at(Coordinate(row_idx, 6), progress_text)

    def finish_update(self) -> None:
        """Called when all updates are complete."""
        self.updating = False
        self.refresh_table()
        self.notify("All updates complete!", severity="information")


def main():
    """Main entry point."""
    app = DependencyUpdaterApp()
    app.run()


if __name__ == '__main__':
    main()
