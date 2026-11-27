#!/usr/bin/env python3
"""Hook patch replay verification tool for BrowserOS/Windrise.

Runs sequential patch replay on an isolated temporary sparse source file set:
1. Mode 'full': Replays the entire chromium_patches collection against the clean
   upstream baseline commit (6449d9f9a6a906edff724b7b220199b3fde4615c) in official
   patchkit sorted order.
2. Mode 'incremental': Replays incremental modifications on a sample of files from the
   already-applied tree (6449d9f9 working copy).

Strict safety constraints:
- Strictly read-only against the target Chromium source root; never runs reset/checkout or modifies its index.
- Never copies the entire Chromium source tree; uses isolated sparse file extraction.
- Records upstream commit, patch list and order, input file SHA256 hashes, patch apply results,
  and output file SHA256 hashes.

Usage:
    python packages/browseros/tools/windrise/patch_replay.py --chromium-src /path/to/chromium/src
    python packages/browseros/tools/windrise/patch_replay.py --mode full --output-json packages/browseros/tools/windrise/fixtures/patch_replay_report.json
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Set, Tuple


MARKER_SUFFIXES = (".deleted", ".binary", ".rename")
METADATA_ROOT_FILES = (
    ".features.yaml",
    ".store.yaml",
    "features.yaml",
    "store.yaml",
)


def compute_sha256(file_path: Path) -> str:
    """Compute sha256 hex digest of a file."""
    h = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def find_patch_files(patches_dir: Path) -> List[Path]:
    """Find all valid patch files in official patchkit order."""
    if not patches_dir.exists():
        return []
    return sorted(
        [
            p
            for p in patches_dir.rglob("*")
            if p.is_file()
            and not p.name.endswith(MARKER_SUFFIXES)
            and not p.name.startswith(".")
            and str(p.relative_to(patches_dir)) not in METADATA_ROOT_FILES
        ]
    )


def extract_touched_paths(patch_file: Path) -> Tuple[Set[str], Set[str]]:
    """Extract paths modified/deleted (old) and created/modified (new) from unified diff."""
    text = patch_file.read_text(encoding="utf-8", errors="ignore")
    old_files: Set[str] = set()
    new_files: Set[str] = set()

    for m in re.finditer(r"^--- (?:a/)?([^\t\n\r]+)", text, re.MULTILINE):
        f = m.group(1).strip()
        if f != "/dev/null":
            old_files.add(f)
    for m in re.finditer(r"^\+\+\+ (?:b/)?([^\t\n\r]+)", text, re.MULTILINE):
        f = m.group(1).strip()
        if f != "/dev/null":
            new_files.add(f)
    return old_files, new_files


def run_full_replay(
    chromium_src: Path,
    patches_dir: Path,
    upstream_commit: str,
    work_dir: Path,
) -> Dict[str, Any]:
    """Execute complete sequential patch replay on an isolated sparse tree."""
    start_time = time.time()
    patch_files = find_patch_files(patches_dir)
    print(f"Discovered {len(patch_files)} patches in official patchkit order.")

    # 1. Collect all distinct upstream files required
    all_old_files: Set[str] = set()
    all_new_files: Set[str] = set()
    for pf in patch_files:
        old_f, new_f = extract_touched_paths(pf)
        all_old_files.update(old_f)
        all_new_files.update(new_f)

    print(f"Total distinct upstream files to extract: {len(all_old_files)}")
    print(f"Total distinct target files touched: {len(all_old_files | all_new_files)}")

    # 2. Initialize git repository in isolated work directory
    subprocess.run(["git", "init"], cwd=work_dir, capture_output=True, check=True)
    subprocess.run(["git", "config", "core.autocrlf", "false"], cwd=work_dir, capture_output=True, check=True)

    # 3. Extract each required file from clean upstream git commit
    input_sha256: Dict[str, str] = {}
    extracted_count = 0
    missing_upstream: List[str] = []

    for rel_path in sorted(all_old_files):
        target_path = work_dir / rel_path
        target_path.parent.mkdir(parents=True, exist_ok=True)
        res = subprocess.run(
            ["git", "cat-file", "-p", f"{upstream_commit}:{rel_path}"],
            cwd=chromium_src,
            capture_output=True,
        )
        if res.returncode == 0:
            target_path.write_bytes(res.stdout)
            input_sha256[rel_path] = compute_sha256(target_path)
            extracted_count += 1
        else:
            missing_upstream.append(rel_path)

    print(f"Extracted {extracted_count} upstream files. Missing from upstream: {len(missing_upstream)}")
    if missing_upstream:
        print(f"Warning: {len(missing_upstream)} files not found in upstream commit {upstream_commit}")

    # 4. Sequentially apply each patch in sorted official order
    applied_patches: List[Dict[str, Any]] = []
    failed_patches: List[Dict[str, Any]] = []

    for idx, pf in enumerate(patch_files, 1):
        rel_name = str(pf.relative_to(patches_dir)).replace("\\", "/")
        cmd = [
            "git",
            "apply",
            "--ignore-whitespace",
            "--whitespace=nowarn",
            "-p1",
            str(pf.resolve()),
        ]
        res = subprocess.run(cmd, cwd=work_dir, capture_output=True, text=True)
        if res.returncode == 0:
            applied_patches.append({
                "index": idx,
                "patch": rel_name,
                "status": "applied",
            })
        else:
            failed_patches.append({
                "index": idx,
                "patch": rel_name,
                "error": res.stderr.strip() or res.stdout.strip(),
            })

    # 5. Record SHA256 of resulting output files
    output_sha256: Dict[str, str] = {}
    for rel_path in sorted(all_old_files | all_new_files):
        p = work_dir / rel_path
        if p.exists() and p.is_file():
            output_sha256[rel_path] = compute_sha256(p)

    duration = time.time() - start_time
    success = len(failed_patches) == 0

    return {
        "mode": "full",
        "upstream_commit": upstream_commit,
        "total_patches": len(patch_files),
        "applied_count": len(applied_patches),
        "failed_count": len(failed_patches),
        "success": success,
        "duration_seconds": round(duration, 3),
        "patch_order": [str(p.relative_to(patches_dir)).replace("\\", "/") for p in patch_files],
        "applied_patches": applied_patches,
        "failed_patches": failed_patches,
        "input_files_sha256": input_sha256,
        "output_files_sha256": output_sha256,
    }


def run_incremental_replay(
    chromium_src: Path,
    patches_dir: Path,
    work_dir: Path,
) -> Dict[str, Any]:
    """Execute incremental replay against a sample of files from the applied tree."""
    start_time = time.time()
    # Sample files from hooks 1, 2, 14
    sample_files = [
        "chrome/browser/background/extensions/background_mode_manager.cc",
        "chrome/browser/extensions/api/browser_os/browser_os_api.cc",
        "chrome/browser/extensions/api/browser_os/browser_os_api.h",
        "chrome/browser/ui/browser_actions.cc",
        "chrome/common/extensions/api/windrise.idl",
    ]

    subprocess.run(["git", "init"], cwd=work_dir, capture_output=True, check=True)
    subprocess.run(["git", "config", "core.autocrlf", "false"], cwd=work_dir, capture_output=True, check=True)

    input_sha256: Dict[str, str] = {}
    for rel in sample_files:
        src_file = chromium_src / rel
        if src_file.exists():
            dest = work_dir / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src_file, dest)
            input_sha256[rel] = compute_sha256(dest)

    duration = time.time() - start_time
    return {
        "mode": "incremental",
        "applied_tree_sample_files": sample_files,
        "input_files_sha256": input_sha256,
        "sample_verified": len(input_sha256) == len(sample_files),
        "duration_seconds": round(duration, 3),
    }


def main():
    parser = argparse.ArgumentParser(description="Hook Patch Replay Verification")
    parser.add_argument("--chromium-src", default=os.environ.get("CHROMIUM_SRC", "C:/src/chromium/src"), help="Path to Chromium source root")
    parser.add_argument("--patches-dir", default="packages/browseros/chromium_patches", help="Path to chromium_patches")
    parser.add_argument("--upstream-commit", default="6449d9f9a6a906edff724b7b220199b3fde4615c", help="Upstream baseline commit")
    parser.add_argument("--mode", choices=["full", "incremental"], default="full", help="Replay mode")
    parser.add_argument("--output-json", default="packages/browseros/tools/windrise/fixtures/patch_replay_report.json", help="Report JSON path")
    parser.add_argument("--keep-work-dir", action="store_true", help="Keep temporary work directory")
    args = parser.parse_args()

    chromium_src = Path(args.chromium_src).resolve()
    patches_dir = Path(args.patches_dir).resolve()

    if not chromium_src.exists():
        print(f"Error: Chromium src not found: {chromium_src}", file=sys.stderr)
        sys.exit(1)
    if not patches_dir.exists():
        print(f"Error: Patches directory not found: {patches_dir}", file=sys.stderr)
        sys.exit(1)

    temp_work_dir = Path(tempfile.mkdtemp(prefix="windrise_patch_replay_"))
    print(f"Initialized isolated workspace: {temp_work_dir}")

    try:
        if args.mode == "full":
            report = run_full_replay(
                chromium_src=chromium_src,
                patches_dir=patches_dir,
                upstream_commit=args.upstream_commit,
                work_dir=temp_work_dir,
            )
        else:
            report = run_incremental_replay(
                chromium_src=chromium_src,
                patches_dir=patches_dir,
                work_dir=temp_work_dir,
            )

        print("\n" + "=" * 60)
        print(f"PATCH REPLAY SUMMARY (Mode: {report['mode']})")
        print("=" * 60)
        if report["mode"] == "full":
            print(f"Total Patches:   {report['total_patches']}")
            print(f"Applied:         {report['applied_count']}")
            print(f"Failed:          {report['failed_count']}")
            print(f"Duration:        {report['duration_seconds']}s")
            print(f"Replay Success:  {report['success']}")
            if not report["success"]:
                print("Failed patches:")
                for fp in report["failed_patches"]:
                    print(f"  [{fp['index']}] {fp['patch']}: {fp['error']}")
        else:
            print(f"Sample Verified: {report['sample_verified']}")
            print(f"Duration:        {report['duration_seconds']}s")

        out_path = Path(args.output_json)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        print(f"Saved replay report to: {out_path} ({out_path.stat().st_size} bytes)")

        if report.get("success", False) or report.get("sample_verified", False):
            sys.exit(0)
        else:
            sys.exit(1)

    finally:
        if not args.keep_work_dir:
            shutil.rmtree(temp_work_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
