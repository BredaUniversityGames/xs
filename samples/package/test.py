#!/usr/bin/env python3
"""
Package the rogue sample and verify the .xs file header contains the expected entries.

Usage:
    python test.py
"""

import io
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

MAGIC_NUMBER = 0x454E49474E455358  # "XSENGINE" little-endian

SUPPORTED_EXTENSIONS = {
    # text (compressed in package)
    ".wren", ".frag", ".vert", ".glsl", ".comp",
    ".json", ".txt", ".xsanim", ".xssprite", ".xstiles",
    # binary (stored as-is)
    ".ttf", ".otf", ".png", ".bank", ".wav", ".mp3", ".jpg", ".ogg", ".flac",
}


def expected_entries(folder: Path) -> set[str]:
    """Mirror the packager's file filtering to build the expected entry set."""
    entries = set()
    for path in folder.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(folder)
        # Skip files inside hidden directories or hidden files themselves
        if any(part.startswith(".") for part in rel.parts):
            continue
        if path.suffix not in SUPPORTED_EXTENSIONS:
            continue
        entries.add("[game]/" + "/".join(rel.parts))
    return entries


def find_xs_exe(repo_root: Path) -> Path | None:
    if sys.platform == "win32":
        candidate = repo_root / "build" / "executable" / "x64" / "Develop" / "xs.exe"
        if candidate.exists():
            return candidate
    else:
        # Linux / macOS: CMake default output
        candidate = repo_root / "build" / "xs"
        if candidate.exists():
            return candidate
        # CLion default
        candidate = repo_root / "cmake-build-debug" / "xs"
        if candidate.exists():
            return candidate
        # Fallback: installed on PATH
        found = shutil.which("xs")
        if found:
            return Path(found)
    return None


# ---------------------------------------------------------------------------
# Header parser — cereal BinaryOutputArchive format
#   size_type is always uint64_t (cereal/details/helpers.hpp)
# ---------------------------------------------------------------------------

def _read(f: io.RawIOBase, n: int) -> bytes:
    data = f.read(n)
    if len(data) != n:
        raise EOFError(f"expected {n} bytes, got {len(data)}")
    return data

def _u32(f):  return struct.unpack("<I", _read(f, 4))[0]
def _u64(f):  return struct.unpack("<Q", _read(f, 8))[0]
def _bool(f): return struct.unpack("?",  _read(f, 1))[0]

def _string(f) -> str:
    length = _u64(f)
    return _read(f, length).decode("utf-8")

def parse_header(path: Path) -> tuple[int, str, list[dict]]:
    """Return (magic, version_str, entries) by reading only the metadata section."""
    with open(path, "rb") as f:
        magic   = _u64(f)
        version = _u32(f)
        count   = _u64(f)

        entries = []
        for _ in range(count):
            relative_path     = _string(f)
            uncompressed_size = _u64(f)
            data_offset       = _u64(f)
            data_length       = _u64(f)
            is_compressed     = _bool(f)
            entries.append({
                "path":              relative_path,
                "uncompressed_size": uncompressed_size,
                "data_offset":       data_offset,
                "data_length":       data_length,
                "is_compressed":     is_compressed,
            })

    year  = (version >> 16) & 0xFFFF
    build = version & 0xFFFF
    return magic, f"{year}.{build}", entries


# ---------------------------------------------------------------------------
# Test
# ---------------------------------------------------------------------------

def run_test(xs_exe: Path, rogue_dir: Path) -> bool:
    ok = True

    with tempfile.TemporaryDirectory() as tmp:
        output = Path(tmp) / "rogue.xs"

        print(f"Packaging {rogue_dir.name}...")
        result = subprocess.run(
            [str(xs_exe), "package", str(rogue_dir), str(output)],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
        if result.returncode != 0:
            print(f"  FAIL  xs package exited with {result.returncode}")
            if result.stderr.strip():
                print(f"        {result.stderr.strip()}")
            return False

        print(f"  parsing header of {output.name} ({output.stat().st_size // 1024} KB)...")

        try:
            magic, version, entries = parse_header(output)
        except Exception as e:
            print(f"  FAIL  could not parse header: {e}")
            return False

        # magic
        if magic == MAGIC_NUMBER:
            print(f"  PASS  magic = 0x{magic:016X}")
        else:
            print(f"  FAIL  magic = 0x{magic:016X}  (expected 0x{MAGIC_NUMBER:016X})")
            ok = False

        # version
        print(f"  INFO  version = {version}  ({len(entries)} entries)")

        # expected files — derived from the rogue folder itself
        required = expected_entries(rogue_dir)
        packed_paths = {e["path"] for e in entries}
        missing = required - packed_paths
        if missing:
            for p in sorted(missing):
                print(f"  FAIL  missing: {p}")
            ok = False
        else:
            print(f"  PASS  all {len(required)} expected entries present")

        # no dotfiles should have leaked through
        bad = [e["path"] for e in entries if any(
            part.startswith(".") for part in e["path"].split("/")
        )]
        if bad:
            for p in bad:
                print(f"  FAIL  dotfile should have been excluded: {p}")
            ok = False

    return ok


def main() -> int:
    repo_root = Path(__file__).parent.parent.parent
    xs_exe = find_xs_exe(repo_root)
    if xs_exe is None:
        print("ERROR: xs executable not found.", file=sys.stderr)
        if sys.platform == "win32":
            print("       Build the Develop configuration first.", file=sys.stderr)
        else:
            print("       Build with CMake or install xs to PATH.", file=sys.stderr)
        return 1

    rogue_dir = Path(__file__).parent.parent / "rogue"
    if not rogue_dir.is_dir():
        print(f"ERROR: rogue sample not found at {rogue_dir}", file=sys.stderr)
        return 1

    passed = run_test(xs_exe, rogue_dir)
    print("\nPASS" if passed else "\nFAIL")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
