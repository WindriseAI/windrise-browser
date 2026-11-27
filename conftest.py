from __future__ import annotations

import os
from collections.abc import Mapping
from pathlib import Path

import pytest

MARKER_NAME = "windrise-no-local-tests"


def resolve_git_common_dir(start: Path) -> Path | None:
    current = start.resolve()
    for candidate in (current, *current.parents):
        dot_git = candidate / ".git"
        if dot_git.is_dir():
            return dot_git
        if dot_git.is_file():
            text = dot_git.read_text(encoding="utf-8", errors="ignore")
            for line in text.splitlines():
                if line.lower().startswith("gitdir:"):
                    raw = line.split(":", 1)[1].strip()
                    git_dir = (
                        Path(raw)
                        if Path(raw).is_absolute()
                        else (candidate / raw).resolve()
                    )
                    common_file = git_dir / "commondir"
                    if common_file.is_file():
                        common_raw = common_file.read_text(
                            encoding="utf-8", errors="ignore"
                        ).strip()
                        if common_raw:
                            return (
                                Path(common_raw)
                                if Path(common_raw).is_absolute()
                                else (git_dir / common_raw).resolve()
                            )
                    return git_dir
    return None


def check_local_test_policy(start: Path, env: Mapping[str, str]) -> None:
    if env.get("WINDRISE_DISABLE_LOCAL_TESTS") == "1":
        raise pytest.UsageError(
            "Local test execution is disabled (WINDRISE_DISABLE_LOCAL_TESTS=1)."
        )
    common_dir = resolve_git_common_dir(start)
    if common_dir is not None and (common_dir / MARKER_NAME).exists():
        raise pytest.UsageError(
            f"Local test execution is disabled ({common_dir / MARKER_NAME} is present)."
        )


check_local_test_policy(Path(__file__).resolve().parent, os.environ)
