#!/usr/bin/env python3
"""Package MaskGame for any of its five target platforms.

This wraps Unreal's RunUAT BuildCookRun, which is the supported way to produce a
standalone build, and fills in the per-platform flags that are easy to forget:
arm64 for Apple Silicon, the ASTC texture flavour for Android, and the
distribution switch that Shipping builds need in order to be signable.

    python3 Scripts/package.py --platform win64
    python3 Scripts/package.py --platform mac --config Shipping
    python3 Scripts/package.py --platform android --output ~/builds
    python3 Scripts/package.py --platform ios --dry-run

Use --dry-run to print the command without running it, which is also how the
tests check this script without an engine installed.
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
UPROJECT = REPO_ROOT / "MaskGame.uproject"

# Our platform names, and what UAT calls them.
PLATFORMS = {
    "win64": "Win64",
    "mac": "Mac",
    "linux": "Linux",
    "android": "Android",
    "ios": "IOS",
}

# Which host operating systems can build for which target. Apple's toolchains
# are the strict ones: only a Mac can produce a Mac or iOS build.
HOST_SUPPORT = {
    "win64": {"Windows"},
    "mac": {"Darwin"},
    "linux": {"Linux", "Windows"},
    "android": {"Windows", "Darwin", "Linux"},
    "ios": {"Darwin"},
}

# Where the engine usually lives, per host, newest first.
ENGINE_GUESSES = {
    "Windows": [
        r"C:\Program Files\Epic Games\UE_5.5",
        r"C:\Program Files\Epic Games\UE_5.4",
    ],
    "Darwin": [
        "/Users/Shared/Epic Games/UE_5.5",
        "/Users/Shared/Epic Games/UE_5.4",
    ],
    "Linux": [
        str(Path.home() / "UnrealEngine"),
        "/opt/UnrealEngine",
    ],
}


def find_engine(explicit: str | None) -> Path:
    """Locate the engine, preferring an explicit path, then UE_ROOT, then guesses."""
    candidates: list[Path] = []
    if explicit:
        candidates.append(Path(explicit).expanduser())
    if os.environ.get("UE_ROOT"):
        candidates.append(Path(os.environ["UE_ROOT"]).expanduser())
    candidates += [Path(p) for p in ENGINE_GUESSES.get(platform.system(), [])]

    for candidate in candidates:
        if (candidate / "Engine" / "Build" / "BatchFiles").is_dir():
            return candidate

    searched = "\n  ".join(str(c) for c in candidates) or "  (nothing to try)"
    raise SystemExit(
        "Could not find Unreal Engine. Pass --engine /path/to/UE_5.5, or set UE_ROOT.\n"
        f"Looked in:\n  {searched}"
    )


def run_uat_path(engine: Path, require_exists: bool = True) -> Path:
    """RunUAT is a .bat on Windows and a .sh everywhere else."""
    batch = engine / "Engine" / "Build" / "BatchFiles"
    script = batch / ("RunUAT.bat" if platform.system() == "Windows" else "RunUAT.sh")
    if require_exists and not script.exists():
        raise SystemExit(f"RunUAT is missing from the engine at {script}")
    return script


def build_command(args: argparse.Namespace, engine: Path, output: Path) -> list[str]:
    uat_platform = PLATFORMS[args.platform]

    command = [
        # A dry run prints the command for an engine that need not be installed.
        str(run_uat_path(engine, require_exists=not args.dry_run)),
        "BuildCookRun",
        f"-project={UPROJECT}",
        "-noP4",
        "-utf8output",
        "-nocompileeditor",
        f"-platform={uat_platform}",
        f"-clientconfig={args.config}",
        "-build",
        "-cook",
        "-stage",
        "-pak",
        "-archive",
        f"-archivedirectory={output}",
    ]

    if args.platform == "mac":
        # Apple Silicon only. Without this UAT builds a universal binary, which
        # doubles the size and the build time for a slice nothing here needs.
        command.append("-architecture=arm64")

    if args.platform == "android":
        # ASTC is the texture format every arm64 Android GPU supports; the other
        # flavours exist for hardware this project's minimum SDK excludes anyway.
        command.append("-cookflavor=ASTC")

    if args.platform == "ios":
        if args.signing_identity:
            command.append(f"-codesignidentity={args.signing_identity}")
        if args.provisioning_profile:
            command.append(f"-provision={args.provisioning_profile}")

    if args.config == "Shipping":
        # Required before a store will accept the build, and it strips the
        # console and the debug commands with it.
        command.append("-distribution")

    if args.clean:
        command.append("-clean")

    command += args.extra
    return command


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Package MaskGame for one of its target platforms.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--platform", required=True, choices=sorted(PLATFORMS),
                        help="Target platform.")
    parser.add_argument("--config", default="Development",
                        choices=["Debug", "Development", "Shipping"],
                        help="Build configuration (default: Development).")
    parser.add_argument("--engine", help="Path to the engine, e.g. /Users/Shared/Epic Games/UE_5.5")
    parser.add_argument("--output", help="Where to put the packaged build (default: Build/<Platform>).")
    parser.add_argument("--clean", action="store_true", help="Force a full rebuild.")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print the command instead of running it.")
    parser.add_argument("--signing-identity", help="iOS only: the code signing identity.")
    parser.add_argument("--provisioning-profile", help="iOS only: the provisioning profile.")
    parser.add_argument("extra", nargs="*", help="Extra flags passed straight through to UAT.")
    args = parser.parse_args()

    if not UPROJECT.exists():
        raise SystemExit(f"Cannot find {UPROJECT}. Run this from a MaskGame checkout.")

    host = platform.system()
    supported = HOST_SUPPORT[args.platform]
    if host not in supported and not args.dry_run:
        raise SystemExit(
            f"A {args.platform} build cannot be made on {host}.\n"
            f"It needs one of: {', '.join(sorted(supported))}."
            + ("\nApple's toolchain is only licensed for macOS." if args.platform in ("mac", "ios") else "")
        )

    output = Path(args.output).expanduser().resolve() if args.output \
        else REPO_ROOT / "Build" / PLATFORMS[args.platform]

    # --dry-run has to work with no engine present, which is what makes this
    # script testable in CI and on a machine that has never seen Unreal.
    if args.dry_run:
        try:
            engine = find_engine(args.engine)
        except SystemExit:
            engine = Path(args.engine or "/path/to/UE_5.5")
    else:
        engine = find_engine(args.engine)

    command = build_command(args, engine, output)

    if args.dry_run:
        print(" \\\n    ".join(command))
        return 0

    output.mkdir(parents=True, exist_ok=True)
    print(f"Packaging {args.platform} ({args.config}) into {output}")
    print(" ".join(command))

    if not shutil.which("mono") and host == "Linux":
        # UAT is a .NET program; on Linux the engine ships its own runtime, so
        # this is a hint rather than a hard failure.
        print("note: no system mono found; relying on the engine's bundled dotnet.")

    result = subprocess.run(command, cwd=REPO_ROOT)
    if result.returncode != 0:
        print(f"\nPackaging failed with exit code {result.returncode}.", file=sys.stderr)
        return result.returncode

    print(f"\nDone. The build is in {output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
