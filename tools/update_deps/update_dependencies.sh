#!/bin/bash
# XS Dependency Updater - Unix/Mac Launcher

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python3 "$SCRIPT_DIR/update_dependencies.py" "$@"
