#!/usr/bin/env python3
"""Check that Scripts/package.py emits the right UAT command for each platform.

Packaging failures are slow to discover - a wrong flag surfaces at the end of a
half-hour build, on a machine that may not be the one you are sitting at. The
script's --dry-run mode prints the command without needing an engine, so the
flags that matter can be asserted here in a few milliseconds.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPT = ROOT / "Scripts" / "package.py"

failures: list[str] = []
checks = 0


def dry_run(*args: str) -> str:
    result = subprocess.run(
        [sys.executable, str(SCRIPT), "--dry-run", "--engine", "/nonexistent/UE_5.5", *args],
        capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(f"package.py failed for {args}:\n{result.stderr}")
    return result.stdout


def check(condition: bool, message: str) -> None:
    global checks
    checks += 1
    if not condition:
        failures.append(message)


def main() -> int:
    expected_platform = {
        "win64": "Win64",
        "mac": "Mac",
        "linux": "Linux",
        "android": "Android",
        "ios": "IOS",
    }

    for name, uat_name in expected_platform.items():
        output = dry_run("--platform", name)

        check(f"-platform={uat_name}" in output,
              f"{name}: command does not target {uat_name}")
        check("BuildCookRun" in output, f"{name}: not a BuildCookRun command")
        check("-clientconfig=Development" in output, f"{name}: wrong default configuration")
        check("MaskGame.uproject" in output, f"{name}: the project is not passed")
        for flag in ("-build", "-cook", "-stage", "-pak", "-archive"):
            check(flag in output, f"{name}: missing {flag}")

        # A Development build must never carry the distribution switch, which
        # strips the console and the debug commands.
        check("-distribution" not in output, f"{name}: Development should not be a distribution build")

        # Apple Silicon only, and only for Mac.
        if name == "mac":
            check("-architecture=arm64" in output, "mac: not pinned to Apple Silicon")
        else:
            check("-architecture=arm64" not in output, f"{name}: should not pin an architecture")

        # ASTC textures, and only for Android.
        if name == "android":
            check("-cookflavor=ASTC" in output, "android: missing the ASTC texture flavour")
        else:
            check("-cookflavor" not in output, f"{name}: should not set a cook flavour")

    # Shipping adds distribution on every platform.
    for name in expected_platform:
        output = dry_run("--platform", name, "--config", "Shipping")
        check("-clientconfig=Shipping" in output, f"{name}: Shipping not applied")
        check("-distribution" in output, f"{name}: Shipping must be a distribution build")

    # An explicit output directory is honoured.
    output = dry_run("--platform", "linux", "--output", "/tmp/maskgame-build")
    check("-archivedirectory=/tmp/maskgame-build" in output, "output directory is ignored")

    # Extra flags pass straight through.
    output = dry_run("--platform", "win64", "--clean")
    check("-clean" in output, "--clean is not forwarded")

    print(f"Packaging: {checks} checks, {len(failures)} failures")
    for failure in failures:
        print(f"    FAIL {failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
