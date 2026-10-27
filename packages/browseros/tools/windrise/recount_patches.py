#!/usr/bin/env python3
"""Recompute every hunk line count in ``chromium_patches`` from the hunk body.

Equivalent to ``git apply --recount``, but persistent: ``git apply --recount``
only tolerates wrong counts while applying and leaves the file on disk broken,
so the next plain ``git apply`` (the build pipeline's patch step) fails with
"corrupt patch". Editing a hunk by hand almost always invalidates the counts,
so this runs after every patch edit.

What is rewritten: the ``@@ -old_start,old_count +new_start,new_count @@``
header of every hunk. Counts come from the hunk body (context, ``+`` and ``-``
lines; a bare empty line is empty context; ``\\ No newline at end of file``
is not a line). ``new_start`` is recomputed from ``old_start`` plus the running
line delta accumulated over the earlier hunks of the same file, following
git's own conventions (a pure creation hunk starts at ``+1``, a pure deletion
hunk points at the line before its empty new range).

What is left alone: everything else, including index lines, file mode lines and
the section heading after the closing ``@@``. Patches git would not parse as a
unified diff (binary hunks, markers) are reported and skipped, never rewritten.
Hunks with no added or removed line are dropped: git apply rejects them as
corrupt patches, and they carry no change.

Usage:
    python packages/browseros/tools/windrise/recount_patches.py
    python packages/browseros/tools/windrise/recount_patches.py --check
    python packages/browseros/tools/windrise/recount_patches.py --git

``--check`` reports the patches that need recounting and exits 1 without
writing. ``--git`` additionally asks ``git apply --stat`` to parse every patch
and reports the ones git rejects, which catches damage this tool cannot see
(binary hunks, truncated files).
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator, List, Optional, Sequence, Tuple

PACKAGE_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_PATCHES_DIR = PACKAGE_ROOT / "chromium_patches"

# Same set the build pipeline skips: marker files annotate a patch path and are
# never applied; dotfiles and the root metadata files are not patches.
# ``batch_apply.find_patch_files`` owns this list for the pipeline; the unit
# test asserts the two stay in agreement.
MARKER_SUFFIXES = (".deleted", ".binary", ".rename")
METADATA_ROOT_FILES = (
    ".features.yaml",
    ".store.yaml",
    "features.yaml",
    "store.yaml",
)

HUNK_HEADER = re.compile(
    r"^@@ -(?P<old_start>\d+)(?:,(?P<old_count>\d+))?"
    r" \+(?P<new_start>\d+)(?:,(?P<new_count>\d+))? @@(?P<section>.*)$"
)
DIFF_HEADER = "diff --git "
HUNK_MARKERS = ("@@ ", "@@-")


class PatchFormatError(Exception):
    """The file is not a unified diff this tool can safely rewrite."""


@dataclass
class Hunk:
    header_line: int
    old_start: int
    old_count: int
    new_start: int
    new_count: int
    section: str
    actual_old: int
    actual_new: int
    # Net lines earlier hunks of the same file added (+) or removed (-).
    delta_before: int

    @property
    def expected_new_start(self) -> int:
        return expected_new_start(
            self.old_start, self.actual_old, self.actual_new, self.delta_before
        )

    @property
    def is_correct(self) -> bool:
        return (
            (self.old_count, self.new_count) == (self.actual_old, self.actual_new)
            and self.new_start == self.expected_new_start
        )


def expected_new_start(
    old_start: int, old_count: int, new_count: int, delta_before: int
) -> int:
    """New-side start line git writes for a hunk starting at ``old_start``.

    Both sides of a hunk header are absolute line numbers in their own file, so
    the new side carries the net line change of every earlier hunk of the same
    file. ``git apply`` ignores this field, which is why a stale value survives
    until something re-diffs the file; git itself always writes it correctly.
    """
    if new_count == 0:
        # Pure deletion: the empty new range sits on the line before it.
        return max(0, old_start + delta_before - 1)
    if old_count == 0:
        # Pure insertion: the empty old range sits before the inserted lines.
        return old_start + delta_before + 1
    return old_start + delta_before


def find_patch_files(patches_dir: Path) -> List[Path]:
    if not patches_dir.exists():
        return []
    return sorted(
        path
        for path in patches_dir.rglob("*")
        if path.is_file()
        and not path.name.endswith(MARKER_SUFFIXES)
        and not path.name.startswith(".")
        and str(path.relative_to(patches_dir)) not in METADATA_ROOT_FILES
    )


def _is_patch(text: str) -> bool:
    return text.startswith(DIFF_HEADER)


def parse_hunks(lines: Sequence[str]) -> List[Hunk]:
    """Collect every hunk in one patch file, with its real line counts."""
    hunks: List[Hunk] = []
    index = 0
    delta = 0
    while index < len(lines):
        line = lines[index]
        if line.startswith("GIT binary patch") or line.startswith("Binary files "):
            # Binary hunks carry encoded payload, not unified-diff lines; a
            # count rewrite there would silently corrupt the patch.
            raise PatchFormatError(
                f"binary patch at line {index + 1}; recount does not apply"
            )
        if line.startswith(DIFF_HEADER):
            # Each file section starts from its own line numbering.
            delta = 0
            index += 1
            continue
        match = HUNK_HEADER.match(lines[index])
        if not match:
            index += 1
            continue

        header_line = index
        old_start = int(match.group("old_start"))
        old_count = int(match.group("old_count") or 1)
        new_start = int(match.group("new_start"))
        new_count = int(match.group("new_count") or 1)
        section = match.group("section")

        index += 1
        actual_old = 0
        actual_new = 0
        while index < len(lines):
            line = lines[index]
            if line.startswith(DIFF_HEADER) or line.startswith(HUNK_MARKERS):
                break
            if line.startswith("\\"):
                # "\ No newline at end of file" annotates the previous line.
                index += 1
                continue
            if line.startswith("+"):
                actual_new += 1
            elif line.startswith("-"):
                actual_old += 1
            elif line.startswith(" ") or line == "":
                # A bare empty line is an empty context line: git strips the
                # trailing space, so tools that hand-edit patches drop it.
                actual_old += 1
                actual_new += 1
            else:
                raise PatchFormatError(
                    f"unexpected hunk line at {index + 1}: {line[:40]!r}"
                )
            index += 1

        hunks.append(
            Hunk(
                header_line=header_line,
                old_start=old_start,
                old_count=old_count,
                new_start=new_start,
                new_count=new_count,
                section=section,
                actual_old=actual_old,
                actual_new=actual_new,
                delta_before=delta,
            )
        )
        delta += actual_new - actual_old
    return hunks


def _format_range(start: int, count: int) -> str:
    # git omits the count when it is 1; match it so a recount of a clean patch
    # is a no-op.
    return f"{start}" if count == 1 else f"{start},{count}"


def drop_empty_hunks(lines: Sequence[str]) -> List[str]:
    """Remove hunks that contain no added or removed line.

    git diff never emits a hunk whose body is entirely context. Such a hunk
    becomes possible after a hand edit removes every ``+``/``-`` line, and
    ``git apply`` rejects the result as a corrupt patch. A no-op hunk has no
    effect, so dropping it is safe.
    """
    result: List[str] = []
    index = 0
    while index < len(lines):
        if not HUNK_HEADER.match(lines[index]):
            result.append(lines[index])
            index += 1
            continue

        start = index
        index += 1
        has_change = False
        while index < len(lines):
            line = lines[index]
            if HUNK_HEADER.match(line) or line.startswith(DIFF_HEADER):
                break
            if line.startswith(("+", "-")):
                has_change = True
            index += 1
        if has_change:
            result.extend(lines[start:index])
    return result


def rewrite_patch(text: str) -> Tuple[str, List[str]]:
    """Return the patch with corrected hunk headers and a change description."""
    lines = text.split("\n")
    trailing_newline = text.endswith("\n")
    if trailing_newline:
        lines.pop()

    lines = drop_empty_hunks(lines)
    hunks = parse_hunks(lines)
    changes: List[str] = []
    for hunk in hunks:
        if hunk.is_correct:
            continue
        new_start = hunk.expected_new_start
        lines[hunk.header_line] = (
            f"@@ -{_format_range(hunk.old_start, hunk.actual_old)} "
            f"+{_format_range(new_start, hunk.actual_new)} @@{hunk.section}"
        )
        changes.append(
            f"line {hunk.header_line + 1}: "
            f"-{hunk.old_start},{hunk.old_count} +{hunk.new_start},{hunk.new_count}"
            f" -> -{hunk.old_start},{hunk.actual_old} +{new_start},{hunk.actual_new}"
        )

    result = "\n".join(lines) + ("\n" if trailing_newline else "")
    return result, changes


def recount_file(path: Path, check_only: bool) -> List[str]:
    """Recount one patch. Returns the change descriptions (empty = clean)."""
    text = path.read_text(encoding="utf-8")
    if not _is_patch(text):
        raise PatchFormatError("not a unified diff")
    updated, changes = rewrite_patch(text)
    if changes and not check_only:
        path.write_text(updated, encoding="utf-8", newline="\n")
    return changes


def iter_git_rejections(paths: Sequence[Path]) -> Iterator[Tuple[Path, str]]:
    """Yield (path, stderr) for patches ``git apply`` refuses to parse."""
    for path in paths:
        result = subprocess.run(
            ["git", "apply", "--stat", "--numstat", str(path)],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            yield path, (result.stderr or result.stdout).strip()


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Recompute chromium patch hunk line counts (git apply --recount, "
            "written back to disk)."
        )
    )
    parser.add_argument(
        "paths",
        nargs="*",
        type=Path,
        help=f"patch files or directories (default: {DEFAULT_PATCHES_DIR})",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="report patches that need recounting; write nothing",
    )
    parser.add_argument(
        "--git",
        action="store_true",
        help="also report patches `git apply` cannot parse",
    )
    args = parser.parse_args(argv)

    targets: List[Path] = []
    for raw in args.paths or [DEFAULT_PATCHES_DIR]:
        path = raw.resolve()
        if path.is_dir():
            targets.extend(find_patch_files(path))
        elif path.is_file():
            targets.append(path)
        else:
            print(f"error: no such path: {raw}", file=sys.stderr)
            return 2

    dirty: List[Tuple[Path, List[str]]] = []
    skipped: List[Tuple[Path, str]] = []
    for path in targets:
        try:
            changes = recount_file(path, check_only=args.check)
        except PatchFormatError as error:
            skipped.append((path, str(error)))
            continue
        except UnicodeDecodeError as error:
            skipped.append((path, f"not UTF-8 text: {error}"))
            continue
        if changes:
            dirty.append((path, changes))

    for path, reason in skipped:
        print(f"skip {path}: {reason}")

    for path, changes in dirty:
        print(f"{'needs recount' if args.check else 'recounted'} {path}")
        for change in changes:
            print(f"  {change}")

    failed = False
    if args.git:
        for path, error in iter_git_rejections(targets):
            failed = True
            print(f"git rejects {path}: {error.splitlines()[0]}")

    print(
        f"\n{len(targets)} patches scanned, {len(dirty)} "
        f"{'need recounting' if args.check else 'recounted'}, "
        f"{len(skipped)} skipped"
    )
    if failed:
        return 1
    return 1 if (args.check and dirty) else 0


if __name__ == "__main__":
    sys.exit(main())
