#!/usr/bin/env python3
"""Tests for the chromium patch recount tool."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

PACKAGE_ROOT = Path(__file__).resolve().parents[2]
for _path in (PACKAGE_ROOT, Path(__file__).resolve().parent):
    if str(_path) not in sys.path:
        sys.path.insert(0, str(_path))

from recount_patches import (  # noqa: E402
    PatchFormatError,
    expected_new_start,
    find_patch_files,
    main,
    parse_hunks,
    rewrite_patch,
)

from bos_build.patchkit import batch_apply  # noqa: E402

HEADER = "diff --git a/f.txt b/f.txt\n--- a/f.txt\n+++ b/f.txt\n"


def _patch(*hunks: str) -> str:
    return HEADER + "".join(hunks)


def _hunk(header: str, *body: str) -> str:
    return header + "\n" + "".join(line + "\n" for line in body)


class RecountRewriteTest(unittest.TestCase):
    def test_correct_patch_is_unchanged(self):
        text = _patch(_hunk("@@ -1,3 +1,3 @@", " a", "-b", "+B", " c"))
        updated, changes = rewrite_patch(text)
        self.assertEqual(updated, text)
        self.assertEqual(changes, [])

    def test_wrong_count_is_rewritten_from_body(self):
        # Declares one context line and one added line; the body has one
        # context line and three added lines.
        text = _patch(_hunk("@@ -1,2 +1,2 @@", " a", "+x", "+y", "+z"))
        updated, changes = rewrite_patch(text)
        self.assertIn("@@ -1 +1,4 @@", updated)
        self.assertEqual(len(changes), 1)

    def test_noop_hunk_is_removed(self):
        # Removing the last added line from a hunk leaves only context; git
        # apply rejects that hunk, so it must disappear entirely.
        text = _patch(_hunk("@@ -1,2 +1,2 @@", " a", " b"))
        updated, _ = rewrite_patch(text)
        self.assertEqual(updated, _patch())

    def test_bare_empty_line_counts_as_context(self):
        # Hand-edited patches lose the trailing space of an empty context line.
        text = _patch(_hunk("@@ -1,3 +1,3 @@", " a", "-b", "+B", "", " c"))
        updated, _ = rewrite_patch(text)
        self.assertIn("@@ -1,4 +1,4 @@", updated)

    def test_no_newline_marker_is_not_a_line(self):
        text = _patch(
            _hunk(
                "@@ -1,2 +1,2 @@",
                " a",
                "-b",
                "+B",
                "\\ No newline at end of file",
            )
        )
        updated, changes = rewrite_patch(text)
        self.assertEqual(updated, text)
        self.assertEqual(changes, [])

    def test_new_start_tracks_earlier_hunks_of_same_file(self):
        # First hunk adds three lines, so the second hunk's new side starts
        # three lines later than its old side.
        text = _patch(
            _hunk("@@ -1,1 +1,1 @@", " a", "+x", "+y", "+z"),
            _hunk("@@ -20,2 +20,2 @@", " p", "-q", "+Q"),
        )
        updated, _ = rewrite_patch(text)
        self.assertIn("@@ -1 +1,4 @@", updated)
        self.assertIn("@@ -20,2 +23,2 @@", updated)

    def test_delta_resets_per_file_section(self):
        second = (
            "diff --git a/g.txt b/g.txt\n"
            "--- a/g.txt\n"
            "+++ b/g.txt\n"
            + _hunk("@@ -1,2 +1,2 @@", " a", "+x", "+y", "+z")
        )
        text = _patch(_hunk("@@ -1,1 +1,1 @@", " a", "+x", "+y", "+z")) + second
        updated, _ = rewrite_patch(text)
        self.assertEqual(updated.count("@@ -1 +1,4 @@"), 2)

    def test_creation_hunk_starts_at_new_line_one(self):
        text = (
            "diff --git a/n.txt b/n.txt\n"
            "new file mode 100644\n"
            "--- /dev/null\n"
            "+++ b/n.txt\n" + _hunk("@@ -0,0 +1,2 @@", "+a", "+b")
        )
        updated, changes = rewrite_patch(text)
        self.assertEqual(updated, text)
        self.assertEqual(changes, [])

    def test_deletion_hunk_points_at_line_before(self):
        text = _patch(_hunk("@@ -5,2 +4,0 @@", "-a", "-b"))
        updated, changes = rewrite_patch(text)
        self.assertEqual(updated, text)
        self.assertEqual(changes, [])

    def test_section_heading_survives(self):
        text = _patch(
            _hunk("@@ -1,1 +1,1 @@ void SomeFunction() {", " a", "+x", "+y")
        )
        updated, _ = rewrite_patch(text)
        self.assertIn("@@ -1 +1,3 @@ void SomeFunction() {", updated)

    def test_binary_patch_is_rejected_not_rewritten(self):
        text = HEADER + "GIT binary patch\nliteral 3\n"
        with self.assertRaises(PatchFormatError):
            rewrite_patch(text)

    def test_parse_reports_actual_counts(self):
        body = _hunk("@@ -3,9 +3,9 @@", " a", "-b", "+B").split("\n")[:-1]
        hunks = parse_hunks(body)
        self.assertEqual(len(hunks), 1)
        self.assertEqual((hunks[0].actual_old, hunks[0].actual_new), (2, 2))


class ExpectedNewStartTest(unittest.TestCase):
    def test_context_only_shifts_with_delta(self):
        self.assertEqual(expected_new_start(10, 4, 4, 0), 10)
        self.assertEqual(expected_new_start(10, 4, 6, 2), 12)
        self.assertEqual(expected_new_start(10, 4, 2, -2), 8)

    def test_pure_insertion(self):
        self.assertEqual(expected_new_start(0, 0, 2, 0), 1)
        self.assertEqual(expected_new_start(5, 0, 2, 0), 6)

    def test_pure_deletion(self):
        self.assertEqual(expected_new_start(5, 2, 0, 0), 4)
        self.assertEqual(expected_new_start(1, 20, 0, 0), 0)


class RecountFileTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)

    def test_check_mode_writes_nothing(self):
        patch = self.root / "p.diff"
        broken = _patch(_hunk("@@ -1,2 +1,2 @@", " a", "+x", "+y"))
        patch.write_text(broken, encoding="utf-8", newline="\n")
        self.assertEqual(main([str(patch), "--check"]), 1)
        self.assertEqual(patch.read_text(encoding="utf-8"), broken)

    def test_recount_then_check_is_clean(self):
        patch = self.root / "p.diff"
        patch.write_text(
            _patch(_hunk("@@ -1,2 +1,2 @@", " a", "+x", "+y")),
            encoding="utf-8",
            newline="\n",
        )
        self.assertEqual(main([str(patch)]), 0)
        self.assertEqual(main([str(patch), "--check"]), 0)

    def test_non_patch_files_are_skipped(self):
        meta = self.root / ".features.yaml"
        meta.write_text("version: '1.0'\n", encoding="utf-8")
        self.assertEqual(main([str(self.root), "--check"]), 0)

    def test_trailing_newline_preserved(self):
        patch = self.root / "p.diff"
        body = _patch(_hunk("@@ -1,2 +1,2 @@", " a", "+x", "+y"))
        patch.write_bytes(body.encode())
        main([str(patch)])
        self.assertTrue(patch.read_bytes().endswith(b"\n"))

    def test_marker_and_metadata_files_are_not_patches(self):
        (self.root / "chrome").mkdir()
        (self.root / "chrome" / "a.cc").write_text("x", encoding="utf-8")
        (self.root / "chrome" / "a.cc.deleted").write_text("x", encoding="utf-8")
        (self.root / ".store.yaml").write_text("x", encoding="utf-8")
        found = [p.name for p in find_patch_files(self.root)]
        self.assertEqual(found, ["a.cc"])

    def test_patch_file_discovery_matches_build_pipeline(self):
        """The tool must scan exactly what the build applies."""
        for name in ("a.cc", "a.cc.deleted", "a.cc.binary", ".hidden", "b.gn"):
            (self.root / name).write_text("x", encoding="utf-8")
        (self.root / ".features.yaml").write_text("x", encoding="utf-8")
        ours = {p.name for p in find_patch_files(self.root)}
        pipeline = {p.name for p in batch_apply.find_patch_files(self.root)}
        self.assertEqual(ours, pipeline)


class RecountAppliesToGitTest(unittest.TestCase):
    """The repaired patch must satisfy `git apply`, not just our own parser."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.repo = self.root / "src"
        self.repo.mkdir()
        self._git("init", "-q", ".")
        self._git("config", "core.autocrlf", "false")
        (self.repo / "f.txt").write_text(
            "".join(f"line{i}\n" for i in range(1, 11)),
            encoding="utf-8",
            newline="\n",
        )
        self._git("add", "f.txt")
        self._git("commit", "-qm", "base")

    def _git(self, *args: str) -> subprocess.CompletedProcess:
        return subprocess.run(
            ["git", *args],
            cwd=self.repo,
            capture_output=True,
            text=True,
            check=False,
            env={
                "GIT_AUTHOR_NAME": "t",
                "GIT_AUTHOR_EMAIL": "t@t",
                "GIT_COMMITTER_NAME": "t",
                "GIT_COMMITTER_EMAIL": "t@t",
                "HOME": str(self.root),
                "PATH": "/usr/bin:/bin:/usr/local/bin",
            },
        )

    def test_broken_patch_is_corrupt_until_recounted(self):
        patch = self.root / "p.diff"
        patch.write_text(
            _patch(
                _hunk(
                    "@@ -1,5 +1,5 @@",
                    " line1",
                    "-line2",
                    "+LINE2",
                    "+inserted",
                    " line3",
                )
            ),
            encoding="utf-8",
            newline="\n",
        )
        broken = self._git("apply", "--check", str(patch))
        self.assertNotEqual(broken.returncode, 0)
        self.assertIn("corrupt patch", broken.stderr)

        self.assertEqual(main([str(patch)]), 0)
        fixed = self._git("apply", "--check", str(patch))
        self.assertEqual(fixed.returncode, 0, fixed.stderr)

    def test_recounted_multi_hunk_patch_applies(self):
        patch = self.root / "p.diff"
        patch.write_text(
            _patch(
                _hunk("@@ -1,3 +1,3 @@", " line1", "-line2", "+LINE2", " line3"),
                _hunk(
                    "@@ -8,3 +8,3 @@", " line8", "-line9", "+LINE9", " line10"
                ),
            ),
            encoding="utf-8",
            newline="\n",
        )
        self.assertEqual(main([str(patch)]), 0)
        applied = self._git("apply", str(patch))
        self.assertEqual(applied.returncode, 0, applied.stderr)
        self.assertIn(
            "LINE9",
            (self.repo / "f.txt").read_text(encoding="utf-8"),
        )

    def test_git_rejects_patches_that_stay_unparseable(self):
        patch = self.root / "p.diff"
        patch.write_text(
            HEADER + "GIT binary patch\n", encoding="utf-8", newline="\n"
        )
        self.assertEqual(main([str(patch), "--git"]), 1)


if __name__ == "__main__":
    unittest.main()
