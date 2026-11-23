#!/usr/bin/env python3
"""Stage a signed windrise-extension directory into a Windrise Browser build.

Validates that the source directory contains manifest.json (with Windrise's
pinned RSA public key), .windrise-integrity and .windrise-signature, verifies
every relative path with browser_directory_zip.check_member_path(), verifies
every file's SHA-256 against .windrise-integrity, and verifies the RSA-SHA256
signature in .windrise-signature against EXPECTED_PUBLIC_KEY. Copies it
into the target windrise_extensions/windrise-extension directory
while keeping bundled_extensions.json as `{}`.

Usage:
    python packages/browseros/tools/windrise/stage_windrise_extension.py \
        --source F:/worktrees/bridge-replica-e45/desktop-integ-1123/electron/windrise-extension \
        --dest D:/src/chromium/src/out/Default_browseros_x64/windrise_extensions/windrise-extension
"""

from __future__ import annotations

import argparse
import shutil
import sys
import tempfile
from pathlib import Path

PACKAGE_ROOT = Path(__file__).resolve().parents[2]
if str(PACKAGE_ROOT) not in sys.path:
    sys.path.insert(0, str(PACKAGE_ROOT))

from bos_build.lib.windrise_extension import verify_source_tree  # noqa: E402


def stage_extension(source_dir: Path, dest_dir: Path, check_only: bool = False) -> int:
    files = verify_source_tree(source_dir)
    if check_only:
        return len(files)

    dest_dir.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        dir=dest_dir.parent, prefix=f".{dest_dir.name}.staging-"
    ) as staging:
        staging_tmp = Path(staging) / dest_dir.name
        for src in files:
            target = staging_tmp / src.relative_to(source_dir)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, target)

        # Validate the bytes going into the build before replacing its bundle.
        verify_source_tree(staging_tmp)
        if dest_dir.is_symlink() or dest_dir.is_junction():
            raise ValueError(f"Links are forbidden at the extension destination: {dest_dir}")
        if dest_dir.exists():
            shutil.rmtree(dest_dir)
        staging_tmp.rename(dest_dir)
    return len(files)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True, help="Signed windrise-extension directory")
    parser.add_argument("--dest", type=Path, help="Destination windrise_extensions/windrise-extension directory")
    parser.add_argument("--check", action="store_true", help="Validate source without copying")
    args = parser.parse_args(argv)

    if not args.check and args.dest is None:
        parser.error("--dest is required unless --check is specified")

    try:
        count = stage_extension(args.source, args.dest or args.source, check_only=args.check)
    except (ValueError, OSError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if args.check:
        print(f"Verified signed windrise-extension ({count} files): {args.source}")
    else:
        print(f"Staged signed windrise-extension ({count} files) -> {args.dest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
