"""Unit tests for 11-21 upstream debranding (U01-U24) in the browser repo.

    python packages/browseros/tools/windrise/debrand_upstream_test.py
"""

import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[3]
PATCHES = HERE.parents[1] / "chromium_patches"
FILES = HERE.parents[1] / "chromium_files"


def read_patch(rel: str) -> str:
    return (PATCHES / rel).read_text(encoding="utf-8")


class DebrandUpstreamTest(unittest.TestCase):
    def test_u01_product_state_dir_and_legacy_migration(self):
        state_cc = read_patch(
            "chrome/browser/browseros/core/browseros_product_state.cc"
        )
        self.assertIn('"WINDRISE_BROWSER_DIR"', state_cc)
        self.assertIn('FILE_PATH_LITERAL(".windrise-browser")', state_cc)
        self.assertNotIn('FILE_PATH_LITERAL(".browseros")', state_cc)

        install_id_cc = read_patch(
            "chrome/browser/browseros/metrics/browseros_installation_id.cc"
        )
        self.assertIn("FindSoleLegacyProductStateDir", install_id_cc)
        self.assertIn("RemoveLegacyProductStateDir", install_id_cc)

        paths_ts = (
            REPO_ROOT
            / "packages/browseros-agent/packages/shared/src/constants/paths.ts"
        ).read_text(encoding="utf-8")
        self.assertIn("'.windrise-browser'", paths_ts)
        self.assertIn("'windrise.sqlite'", paths_ts)

        dir_ts = (
            REPO_ROOT
            / "packages/browseros-agent/apps/server/src/lib/browseros-dir.ts"
        ).read_text(encoding="utf-8")
        self.assertIn("WINDRISE_BROWSER_DIR", dir_ts)

    def test_u03_server_execution_dir_uses_windrise_server(self):
        for rel in (
            "chrome/browser/browseros/server/browseros_server_manager.cc",
            "chrome/browser/browseros/server/browseros_server_utils.cc",
        ):
            text = read_patch(rel)
            self.assertIn('FILE_PATH_LITERAL(".windrise-server")', text)
            self.assertNotIn('FILE_PATH_LITERAL(".browseros")', text)

    def test_u05_server_disabled_by_default_and_switches_rebranded(self):
        switches = read_patch(
            "chrome/browser/browseros/core/browseros_switches.h"
        )
        self.assertIn('"enable-windrise-server"', switches)
        self.assertIn('"disable-windrise-server"', switches)
        self.assertIn('"disable-browseros-server"', switches)
        self.assertIn('"windrise-cdp-port"', switches)
        self.assertIn('"windrise-dock-icon"', switches)

        mgr = read_patch(
            "chrome/browser/browseros/server/browseros_server_manager.cc"
        )
        self.assertIn("!command_line->HasSwitch(browseros::kEnableServer)", mgr)
        self.assertIn("browseros::kLegacyDisableServer", mgr)

    def test_u09_branding_copyright_has_no_upstream(self):
        for rel in (
            "products/browseros/chrome/app/theme/chromium/BRANDING.release",
            "products/browseros/chrome/app/theme/chromium/BRANDING.debug",
            "products/browseros/chrome/updater/branding.gni",
        ):
            text = (FILES / rel).read_text(encoding="utf-8")
            self.assertNotIn("Based on BrowserOS", text)
            self.assertIn("Windrise", text)

    def test_u11_install_progress_window_class_rebranded(self):
        ui_cc = read_patch("chrome/installer/setup/browseros_install_ui.cc")
        self.assertIn('L"WindriseInstallProgressWindow"', ui_cc)
        self.assertNotIn('L"BrowserOSInstallProgressWindow"', ui_cc)


if __name__ == "__main__":
    unittest.main()
