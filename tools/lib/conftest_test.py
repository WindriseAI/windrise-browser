from __future__ import annotations

import importlib.util
import sys
import tempfile
import types
import unittest
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[2]
ROOT_CONFTEST = ROOT_DIR / "conftest.py"


def _load_conftest_module():
    injected_stub = False
    if "pytest" not in sys.modules:
        try:
            import pytest  # noqa: F401
        except ImportError:
            stub = types.ModuleType("pytest")

            class UsageError(Exception):
                pass

            stub.UsageError = UsageError
            sys.modules["pytest"] = stub
            injected_stub = True

    spec = importlib.util.spec_from_file_location("_test_root_conftest", ROOT_CONFTEST)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(module)
    except Exception:
        # Allowed when invoked on a host where the shared marker file exists.
        pass
    finally:
        if injected_stub:
            sys.modules.pop("pytest", None)
    return module


class ConftestPolicyTest(unittest.TestCase):
    def test_allows_when_unblocked(self):
        mod = _load_conftest_module()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / ".git").mkdir()
            mod.check_local_test_policy(root, {})

    def test_blocks_env_disable_flag(self):
        mod = _load_conftest_module()
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(Exception) as ctx:
                mod.check_local_test_policy(Path(tmp), {"WINDRISE_DISABLE_LOCAL_TESTS": "1"})
            self.assertIn("WINDRISE_DISABLE_LOCAL_TESTS=1", str(ctx.exception))

    def test_blocks_shared_worktree_marker(self):
        mod = _load_conftest_module()
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            common_git = base / "repo" / ".git"
            wt_git = common_git / "worktrees" / "wt1"
            wt_dir = base / "wt1"
            wt_git.mkdir(parents=True)
            wt_dir.mkdir(parents=True)
            (wt_git / "commondir").write_text("../..\n", encoding="utf-8")
            (common_git / mod.MARKER_NAME).write_text("1\n", encoding="utf-8")
            (wt_dir / ".git").write_text(f"gitdir: {wt_git}\n", encoding="utf-8")
            self.assertEqual(mod.resolve_git_common_dir(wt_dir), common_git.resolve())
            with self.assertRaises(Exception):
                mod.check_local_test_policy(wt_dir, {})


if __name__ == "__main__":
    unittest.main()
