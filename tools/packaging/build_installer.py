"""Build the Windows xs installer from a staged relocatable bundle.

This is the Windows backend of the xs packaging tool. It does NOT build xs and
it does NOT stage the bundle -- it consumes the bundle your existing packaging
step already produces (bin\\xs.exe + co-located DLLs + lib\\ + share\\xs\\) and
wraps it in a per-user Inno Setup installer that puts bin\\ on the user PATH.

Run on Windows with Inno Setup 6.3+ available (ISCC.exe). Usage:

    python build_installer.py \\
        --bundle-dir   dist/bundle \\
        --output-dir   dist \\
        --version      1.4.0 \\
        --app-id       7c6de762-424d-46f4-8693-444a1b24e69d \\
        --publisher    "Bojan Endrovski" \\
        --exe-name     xs.exe
"""

from __future__ import annotations

import argparse
import dataclasses
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
TEMPLATE = HERE / "windows_installer.iss.in"


@dataclasses.dataclass(frozen=True)
class WindowsPackage:
    app_name: str
    version: str          # e.g. "1.4.0" (no leading "v")
    publisher: str
    app_id: str           # stable GUID *without* surrounding braces
    exe_name: str         # e.g. "xs.exe"
    bundle_dir: Path      # staged relocatable bundle (contains bin\, lib\, share\)
    output_dir: Path      # where the .exe installer is written

    @property
    def output_basename(self) -> str:
        return f"{self.app_name}-setup-{self.version}-x64"

    @property
    def installer_path(self) -> Path:
        return self.output_dir / f"{self.output_basename}.exe"

    def tokens(self) -> dict[str, str]:
        # Inno wants backslash paths; pass absolute so ISCC's CWD is irrelevant.
        return {
            "APP_NAME": self.app_name,
            "APP_VERSION": self.version,
            "PUBLISHER": self.publisher,
            "APP_ID": self.app_id,
            "EXE_NAME": self.exe_name,
            "BUNDLE_DIR": str(self.bundle_dir.resolve()),
            "OUTPUT_DIR": str(self.output_dir.resolve()),
            "OUTPUT_BASENAME": self.output_basename,
        }


def find_iscc() -> Path:
    """Locate ISCC.exe: explicit env var, then PATH, then default install dir."""
    env = os.environ.get("INNO_ISCC")
    if env and Path(env).is_file():
        return Path(env)

    on_path = shutil.which("iscc") or shutil.which("ISCC")
    if on_path:
        return Path(on_path)

    for base in (
        os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
        os.environ.get("ProgramFiles", r"C:\Program Files"),
    ):
        candidate = Path(base) / "Inno Setup 6" / "ISCC.exe"
        if candidate.is_file():
            return candidate

    raise FileNotFoundError(
        "ISCC.exe not found. Install Inno Setup 6.3+ "
        "(e.g. `choco install innosetup`) or set INNO_ISCC."
    )


def render_script(pkg: WindowsPackage, work_dir: Path) -> Path:
    """Substitute @TOKEN@ markers and write the concrete .iss next to the build."""
    text = TEMPLATE.read_text(encoding="utf-8")
    for key, value in pkg.tokens().items():
        text = text.replace(f"@{key}@", value)

    leftover = [w for w in ("@APP_NAME@", "@APP_ID@", "@BUNDLE_DIR@") if w in text]
    if leftover:
        raise RuntimeError(f"Unsubstituted tokens remain: {leftover}")

    work_dir.mkdir(parents=True, exist_ok=True)
    script = work_dir / "xs.iss"
    script.write_text(text, encoding="utf-8")
    return script


def build(pkg: WindowsPackage, work_dir: Path | None = None) -> Path:
    """Render the script, compile it, and return the path to the installer."""
    exe = pkg.bundle_dir / "bin" / pkg.exe_name
    if not exe.is_file():
        raise FileNotFoundError(
            f"Expected {exe} in the bundle. Stage the bundle before packaging."
        )

    pkg.output_dir.mkdir(parents=True, exist_ok=True)
    work_dir = work_dir or (pkg.output_dir / "_iss")
    script = render_script(pkg, work_dir)
    iscc = find_iscc()

    # /Qp = quiet but show errors. ISCC returns non-zero on failure -> check=True.
    subprocess.run([str(iscc), "/Qp", str(script)], check=True)

    if not pkg.installer_path.is_file():
        raise RuntimeError(f"ISCC reported success but {pkg.installer_path} is missing")
    return pkg.installer_path


def _parse_args(argv: list[str]) -> WindowsPackage:
    p = argparse.ArgumentParser(description="Build the Windows xs installer.")
    p.add_argument("--bundle-dir", required=True, type=Path)
    p.add_argument("--output-dir", required=True, type=Path)
    p.add_argument("--version", required=True)
    p.add_argument("--app-id", required=True, help="Stable GUID, no braces")
    p.add_argument("--publisher", required=True)
    p.add_argument("--app-name", default="xs")
    p.add_argument("--exe-name", default="xs.exe")
    a = p.parse_args(argv)

    return WindowsPackage(
        app_name=a.app_name,
        version=a.version.lstrip("v"),   # tolerate a "v1.4.0" git tag
        publisher=a.publisher,
        app_id=a.app_id.strip("{}"),     # tolerate a braced GUID
        exe_name=a.exe_name,
        bundle_dir=a.bundle_dir,
        output_dir=a.output_dir,
    )


def main(argv: list[str]) -> int:
    pkg = _parse_args(argv)
    installer = build(pkg)
    print(f"Built {installer}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
