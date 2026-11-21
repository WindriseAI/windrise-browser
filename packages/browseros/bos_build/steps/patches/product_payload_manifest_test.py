#!/usr/bin/env python3
"""Tests for product payload manifest patches."""

import re
import unittest

from ...lib.paths import get_package_root


PATCHES = get_package_root() / "chromium_patches"


def _patched_source(relative_path: str) -> str:
    """Reconstruct the changed source regions from a unified diff."""
    source_lines: list[str] = []
    in_hunk = False

    for line in (PATCHES / relative_path).read_text().splitlines():
        if line.startswith("@@"):
            in_hunk = True
            continue
        if not in_hunk:
            continue
        if line.startswith("diff --git "):
            in_hunk = False
            continue
        if line.startswith(("+", " ")):
            source_lines.append(line[1:])

    return "\n".join(source_lines)


def _source_literals(source: str) -> set[str]:
    return set(re.findall(r'"([^"\n]+\.(?:crx|json))"', source))


def _conditional_sources(source: str, condition: str) -> set[str]:
    match = re.search(
        rf"if \({re.escape(condition)}\) \{{\n(?P<body>.*?)\n\}}",
        source,
        re.DOTALL,
    )
    if match is None:
        raise AssertionError(f"missing GN condition: {condition}")
    return _source_literals(match.group("body"))


class ProductPayloadManifestPatchTest(unittest.TestCase):
    def test_windows_manifest_excludes_upstream_server_layouts(self) -> None:
        manifest = _patched_source("chrome/installer/mini_installer/chrome.release")
        self.assertNotIn("BrowserOSServer", manifest)
        self.assertNotIn("BrowserClawServer", manifest)

    def test_windows_manifest_carries_bundled_extensions(self) -> None:
        manifest = _patched_source("chrome/installer/mini_installer/chrome.release")
        lines = manifest.splitlines()

        self.assertIn(
            "windrise_extensions\\bundled_extensions.json: %(VersionDir)s\\windrise_extensions\\",
            lines,
        )
        self.assertNotIn(
            "browseros_extensions\\bundled_extensions.json: %(VersionDir)s\\browseros_extensions\\",
            lines,
        )
        self.assertNotIn(
            "windrise_extensions\\*.*: %(VersionDir)s\\windrise_extensions\\",
            lines,
        )

    def test_gn_sources_follow_product_matrix(self) -> None:
        build = _patched_source("chrome/browser/browseros/bundled_extensions/BUILD.gn")
        assignment = re.search(
            r"_bundled_extensions_sources = \[(?P<body>.*?)\n\]",
            build,
            re.DOTALL,
        )
        self.assertIsNotNone(assignment)
        assert assignment is not None

        manifest = "bundled_extensions.json"
        bug_reporter = "adlpneommgkgeanpaekgoaolcpncohkf.crx"
        browserclaw = "pjimfkbpehlcllblajnpfamdfjhhlgkc.crx"
        retired_agent = "bflpfmnmnokmjhmgnolecpppdbdophmk.crx"
        base_sources = _source_literals(assignment.group("body"))
        browserclaw_sources = _conditional_sources(
            build,
            "browseros_allow_runtime_product_override || browseros_product_browserclaw",
        )

        self.assertIn('import("//chrome/browser/browseros/buildflags.gni")', build)
        self.assertEqual(base_sources, {manifest})
        self.assertNotIn(bug_reporter, build)
        self.assertNotIn(retired_agent, build)
        self.assertEqual(browserclaw_sources, {browserclaw})
        self.assertEqual(
            base_sources | browserclaw_sources,
            {manifest, browserclaw},
        )


if __name__ == "__main__":
    unittest.main()
