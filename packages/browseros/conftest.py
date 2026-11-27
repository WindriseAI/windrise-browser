from __future__ import annotations

import importlib.util
import os
from pathlib import Path

import pytest

_ROOT_CONFTEST = Path(__file__).resolve().parents[2] / "conftest.py"
if _ROOT_CONFTEST.is_file():
    _spec = importlib.util.spec_from_file_location("_repo_root_conftest", _ROOT_CONFTEST)
    if _spec is not None and _spec.loader is not None:
        _mod = importlib.util.module_from_spec(_spec)
        _spec.loader.exec_module(_mod)
        _mod.check_local_test_policy(Path(__file__).resolve().parent, os.environ)
elif os.environ.get("WINDRISE_DISABLE_LOCAL_TESTS") == "1":
    raise pytest.UsageError(
        "Local test execution is disabled (WINDRISE_DISABLE_LOCAL_TESTS=1)."
    )
