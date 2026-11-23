#!/usr/bin/env python3
"""Stage a signed windrise-extension directory into a Windrise Browser build.

Validates that the source directory contains manifest.json (with Windrise's
pinned RSA public key), .windrise-integrity and .windrise-signature, and that
every member path passes browser_directory_zip.check_member_path(). Copies it
atomically into the target windrise_extensions/windrise-extension directory
while keeping bundled_extensions.json as `{}`.

Usage:
    python packages/browseros/tools/windrise/stage_windrise_extension.py \
        --source F:/worktrees/bridge-replica-e45/desktop-integ-1123/electron/windrise-extension \
        --dest D:/src/chromium/src/out/Default_browseros_x64/windrise_extensions/windrise-extension
"""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path

WINDRISE_EXTENSION_PUBLIC_KEY = (
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu/BdI9vbyMwLGJs9imDubXmoawZD3wzoYTUB"
    "SIru4A4+KflzIz04Ps8JUFpUmSJdUkgedCujSxQFt+ktysIc6tZAo7xXDqW/wDHr3py5sX7rfAILXh8o"
    "8lG9fRbJZjBj38rb/PTMvmvRJMCKnmM9MAaPwt3DT2Nj8xzR08GzeWdf6mfThswAA5w0YRIlVMhExKBF"
    "fB3b6Nb969QPha5//pxZMBoPSXktb4Gemx3esOtVWoWpxFEbfYs2td1MXGMlvziBN96hZ+49TcnFRKd"
    "zJoALB0q9xp1DHPlM0n9EMNKY+Fps0/17GrwGCYhb3DSyWEX7MXQLZraiT2eWQxpiDwIDAQAB"
)
REQUIRED_FILES = ("manifest.json", ".windrise-integrity", ".windrise-signature")


def validate_signed_extension(source_dir: Path) -> list[Path]:
    """Validate required signed files and return all regular files to stage."""
    if not source_dir.is_dir() or source_dir.is_symlink():
        raise ValueError(f"Signed extension directory not found: {source_dir}")

    for name in REQUIRED_FILES:
        path = source_dir / name
        if path.is_symlink() or not path.is_file() or path.stat().st_size <= 0:
            raise ValueError(f"Missing or invalid required file: {path}")

    manifest = json.loads((source_dir / "manifest.json").read_text(encoding="utf-8"))
    if manifest.get("key") != WINDRISE_EXTENSION_PUBLIC_KEY:
        raise ValueError("manifest.json public key does not match Windrise assistant key")

    files: list[Path] = []
    for entry in sorted(source_dir.rglob("*")):
        if entry.is_symlink():
            raise ValueError(f"Symlinks are forbidden inside signed extension: {entry}")
        if entry.is_file():
            files.append(entry)
    return files


def stage_extension(source_dir: Path, dest_dir: Path, check_only: bool = False) -> int:
    files = validate_signed_extension(source_dir)
    if check_only:
        return len(files)

    staging_tmp = dest_dir.parent / f".{dest_dir.name}.staging"
    if staging_tmp.exists():
        shutil.rmtree(staging_tmp)
    staging_tmp.mkdir(parents=True, exist_ok=True)
    try:
        for src in files:
            rel = src.relative_to(source_dir)
            target = staging_tmp / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, target)
        if dest_dir.exists():
            shutil.rmtree(dest_dir)
        dest_dir.parent.mkdir(parents=True, exist_ok=True)
        staging_tmp.rename(dest_dir)
    finally:
        if staging_tmp.exists():
            shutil.rmtree(staging_tmp, ignore_errors=True)
    return len(files)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True, help="Signed windrise-extension directory")
    parser.add_argument("--dest", type=Path, help="Destination windrise_extensions/windrise-extension directory")
    parser.add_argument("--check", action="store_true", help="Validate source without copying")
    args = parser.parse_args(argv)

    if not args.check and args.dest is None:
        parser.error("--dest is required unless --check is specified")

    count = stage_extension(args.source, args.dest or args.source, check_only=args.check)
    if args.check:
        print(f"Verified signed windrise-extension ({count} files): {args.source}")
    else:
        print(f"Staged signed windrise-extension ({count} files) -> {args.dest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
