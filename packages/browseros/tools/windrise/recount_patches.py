#!/usr/bin/env python3
"""Check that every unified diff under chromium_patches/ is internally valid.

Chromium is not checked out on most machines, so the only thing that can be
verified locally is the patch itself: that each ``@@ -a,b +c,d @@`` count matches
the number of context/removed/added lines actually present, that hunks are
ordered and do not overlap, and that the per-file headers agree with the hunk
headers. A hand-edited hunk whose body changed but whose header did not is the
classic way to break the Chromium build on the machine that finally applies the
stack.

Any other line-count proof (e.g. "I counted by hand") is not a proof; run this
before committing patch edits:

    python packages/browseros/tools/windrise/recount_patches.py --check

Pass paths to check a subset:

    python .../recount_patches.py --check packages/browseros/chromium_patches/chrome/common

Exit code is 0 when all patches are consistent, 1 otherwise.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator, List, Optional, Sequence

# Files in chromium_patches/ that are not patches (metadata / overlays) are
# detected by content: a patch starts with "diff --git ".
PATCH_PREFIX = "diff --git "

HUNK_RE = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@(?: (.*))?$")
GIT_HEADER_RE = re.compile(r"^diff --git a/(\S+) b/(\S+)$")
MINUS_HEADER_RE = re.compile(r"^--- (?:a/)?(\S+)")
PLUS_HEADER_RE = re.compile(r"^\+\+\+ (?:b/)?(\S+)")


@dataclass
class Hunk:
    line_no: int  # 1-based line number of the "@@" line in the patch file
    header: str
    old_start: int
    old_count: int
    new_start: int
    new_count: int
    context: int = 0
    removed: int = 0
    added: int = 0

    @property
    def old_actual(self) -> int:
        return self.context + self.removed

    @property
    def new_actual(self) -> int:
        return self.context + self.added

    @property
    def old_end(self) -> int:
        """Exclusive end line (1-based, last covered line = old_end - 1)."""
        return self.old_start + self.old_count

    def problems(self) -> List[str]:
        errors = []
        if self.old_actual != self.old_count:
            errors.append(
                f"hunk at line {self.line_no}: header says {self.old_count} old "
                f"line(s), body has {self.old_actual} (context {self.context} + "
                f"removed {self.removed})"
            )
        if self.new_actual != self.new_count:
            errors.append(
                f"hunk at line {self.line_no}: header says {self.new_count} new "
                f"line(s), body has {self.new_actual} (context {self.context} + "
                f"added {self.added})"
            )
        return errors


@dataclass
class FileDiff:
    old_path: str
    new_path: str
    header_line: int
    hunks: List[Hunk]


class PatchError(Exception):
    pass


def iter_patch_files(root: Path, explicit: Sequence[Path]) -> Iterator[Path]:
    if explicit:
        for path in explicit:
            if path.is_dir():
                yield from (
                    p
                    for p in sorted(path.rglob("*"))
                    if p.is_file() and _is_patch(p)
                )
            elif path.is_file():
                yield path
        return
    yield from (
        p for p in sorted(root.rglob("*")) if p.is_file() and _is_patch(p)
    )


def _is_patch(path: Path) -> bool:
    if path.name.startswith("."):
        return False
    try:
        with path.open("rb") as handle:
            head = handle.read(len(PATCH_PREFIX))
    except OSError:
        return False
    return head.decode("utf-8", "replace") == PATCH_PREFIX


def parse_patch(text: str) -> List[FileDiff]:
    """Parse a multi-file unified diff and validate hunk bodies.

    Raises PatchError on malformed structure (missing headers, a hunk before
    any file header, body lines outside a hunk, truncated final hunk).
    """
    lines = text.splitlines()
    diffs: List[FileDiff] = []
    current: Optional[FileDiff] = None
    hunk: Optional[Hunk] = None
    seen_minus = seen_plus = False

    def close_hunk() -> None:
        nonlocal hunk, seen_minus, seen_plus
        if hunk is not None and current is not None:
            current.hunks.append(hunk)
        hunk = None
        seen_minus = seen_plus = False

    index = 0
    while index < len(lines):
        raw = lines[index]
        line_no = index + 1

        if raw.startswith("diff --git "):
            close_hunk()
            match = GIT_HEADER_RE.match(raw)
            if not match:
                raise PatchError(
                    f"line {line_no}: malformed 'diff --git' header: {raw!r}"
                )
            old_path, new_path = match.groups()
            if old_path != new_path:
                raise PatchError(
                    f"line {line_no}: per-file patch renames "
                    f"{old_path} -> {new_path}; chromium_patches/ is one file "
                    f"per path"
                )
            current = FileDiff(old_path, new_path, line_no, [])
            diffs.append(current)
            seen_minus = seen_plus = False
            index += 1
            continue

        if current is None:
            raise PatchError(f"line {line_no}: content before 'diff --git' header")

        match = HUNK_RE.match(raw)
        if match:
            if hunk is not None:
                close_hunk()
            old_start, old_count, new_start, new_count = match.groups()[:4]
            hunk = Hunk(
                line_no=line_no,
                header=raw,
                old_start=int(old_start),
                old_count=int(old_count) if old_count is not None else 1,
                new_start=int(new_start),
                new_count=int(new_count) if new_count is not None else 1,
            )
            index += 1
            # Consume the hunk body: ' ', '+', '-', '\' and empty lines (some
            # tools strip the single space of a blank context line).
            while index < len(lines):
                body = lines[index]
                if body.startswith("diff --git ") or HUNK_RE.match(body):
                    break
                if body.startswith("\\"):
                    index += 1
                    continue
                if body == "":
                    # A stripped blank context line: only valid while the hunk
                    # still needs old/new lines.
                    if hunk.old_actual >= hunk.old_count and hunk.new_actual >= hunk.new_count:
                        break
                    hunk.context += 1
                elif body[0] == " ":
                    hunk.context += 1
                elif body[0] == "-":
                    hunk.removed += 1
                elif body[0] == "+":
                    hunk.added += 1
                else:
                    break
                index += 1
            continue

        if raw.startswith("---"):
            match = MINUS_HEADER_RE.match(raw)
            if match and match.group(1) not in ("/dev/null",) \
                    and match.group(1) != current.old_path:
                raise PatchError(
                    f"line {line_no}: '---' path {match.group(1)} does not match "
                    f"'diff --git' path {current.old_path}"
                )
            seen_minus = True
            index += 1
            continue

        if raw.startswith("+++"):
            match = PLUS_HEADER_RE.match(raw)
            if match and match.group(1) not in ("/dev/null",) \
                    and match.group(1) != current.new_path:
                raise PatchError(
                    f"line {line_no}: '+++' path {match.group(1)} does not match "
                    f"'diff --git' path {current.new_path}"
                )
            seen_plus = True
            index += 1
            continue

        if raw.startswith("index ") or raw.startswith("new file mode ") \
                or raw.startswith("deleted file mode ") or raw.startswith("old mode ") \
                or raw.startswith("new mode ") or raw.startswith("similarity index ") \
                or raw.startswith("rename from ") or raw.startswith("rename to "):
            index += 1
            continue

        if hunk is not None:
            # Body line we could not classify (e.g. missing leading space).
            raise PatchError(
                f"line {line_no}: unparsable hunk body line: {raw!r}"
            )

        raise PatchError(f"line {line_no}: unexpected line: {raw!r}")

    close_hunk()

    for diff in diffs:
        if not diff.hunks:
            raise PatchError(
                f"line {diff.header_line}: {diff.old_path} has no hunks"
            )
        previous: Optional[Hunk] = None
        for hunk in diff.hunks:
            if previous is not None and hunk.old_start <= previous.old_end - 1:
                raise PatchError(
                    f"{diff.old_path}: hunk at line {hunk.line_no} starts at old "
                    f"line {hunk.old_start} but previous hunk covers up to "
                    f"{previous.old_end - 1}"
                )
            previous = hunk
    return diffs


def check_file(path: Path, rel: str) -> tuple[List[str], List[str], int]:
    """Return (structural errors, stale-header warnings, hunk count)."""
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        return [f"{rel}: not UTF-8: {exc}"], [], 0
    try:
        diffs = parse_patch(text)
    except PatchError as exc:
        return [f"{rel}: {exc}"], [], 0
    errors: List[str] = []
    warnings: List[str] = []
    hunks = 0
    for diff in diffs:
        for hunk in diff.hunks:
            hunks += 1
            for problem in hunk.problems():
                warnings.append(f"{rel}: {diff.old_path}: {problem}")
    return errors, warnings, hunks


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="validate only (default behavior); never rewrites files",
    )
    parser.add_argument(
        "paths",
        nargs="*",
        type=Path,
        help=(
            "patch files or directories; defaults to chromium_patches/. "
            "Naming paths makes stale header counts fatal for those files."
        ),
    )
    parser.add_argument(
        "--patches-dir",
        type=Path,
        default=None,
        help="chromium_patches/ root used when no paths are given",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="treat stale @@ header counts as errors everywhere, not warnings",
    )
    args = parser.parse_args(argv)

    root = args.patches_dir
    if root is None:
        root = Path(__file__).resolve().parents[2] / "chromium_patches"
    if not root.is_dir():
        print(f"error: patches directory not found: {root}", file=sys.stderr)
        return 1

    explicit = [p.resolve() for p in args.paths]
    files = list(iter_patch_files(root.resolve(), explicit))
    if not files:
        print(f"error: no patch files found under {root}", file=sys.stderr)
        return 1
    strict = args.strict or bool(explicit)

    errors: List[str] = []
    warnings: List[str] = []
    hunks = 0
    for path in files:
        try:
            rel = path.resolve().relative_to(root.resolve()).as_posix()
        except ValueError:
            rel = path.as_posix()
        file_errors, file_warnings, file_hunks = check_file(path, rel)
        errors.extend(file_errors)
        warnings.extend(file_warnings)
        if not file_errors:
            hunks += file_hunks

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
    for warning in warnings:
        print(f"warning: {warning}", file=sys.stderr)

    if errors or (strict and warnings):
        print(
            f"\n{len(errors)} error(s), {len(warnings)} stale header count(s) "
            f"in {len(files)} patch file(s).",
            file=sys.stderr,
        )
        return 1

    if warnings:
        print(
            f"ok: {len(files)} patch file(s), {hunks} hunk(s) structurally "
            f"valid; {len(warnings)} pre-existing stale @@ header count(s) "
            f"(reported above; pass --strict or name the files to fail on them)"
        )
        return 0

    print(f"ok: {len(files)} patch file(s), {hunks} hunk(s), headers consistent")
    return 0


if __name__ == "__main__":
    sys.exit(main())
