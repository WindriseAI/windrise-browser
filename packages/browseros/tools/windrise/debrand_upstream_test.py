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

    def test_u12_zh_cn_product_name_is_windrise_cn(self):
        xtb = read_patch("chrome/app/resources/chromium_strings_zh-CN.xtb")
        added = [l for l in xtb.splitlines() if l.startswith("+")]
        self.assertTrue(any("关于风桥浏览器" in l for l in added))
        self.assertTrue(any("- 风桥浏览器</translation>" in l for l in added))
        self.assertFalse(any("Windrise Browser" in l for l in added))
        self.assertFalse(any("风桥浏览器 云端硬盘" in l for l in added))
        self.assertFalse(any("风桥浏览器浏览器" in l for l in added))

    def test_u13_source_txt_clean_and_version_aligned(self):
        src = read_patch("chrome/installer/mini_installer/SOURCE.txt")
        self.assertNotIn("BrowserOS", src)
        self.assertIn("158.0.8127.41", src)
        self.assertIn("3479de84ee", src)
        self.assertIn("GNU Affero General Public License v3.0", src)
        self.assertIn("https://github.com/WindriseAI/windrise-browser", src)

    def test_u14_about_page_agpl_notice_and_license_viewer(self):
        html_ts = read_patch(
            "chrome/browser/resources/settings/about_page/about_page.html.ts"
        )
        self.assertIn('id="agplLicense"', html_ts)
        self.assertIn("$i18n{aboutAgplNotice}", html_ts)

        ts = read_patch(
            "chrome/browser/resources/settings/about_page/about_page.ts"
        )
        self.assertIn("chrome://resources/windrise_license/LICENSE.txt", ts)

        provider = read_patch(
            "chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc"
        )
        self.assertIn("aboutAgplNotice", provider)
        self.assertIn("AGPL-3.0", provider)
        self.assertIn("不提供任何担保", provider)
        self.assertIn("WITHOUT ANY WARRANTY", provider)

        root_lic = (REPO_ROOT / "LICENSE").read_bytes()
        webui_lic = (
            REPO_ROOT / "packages/browseros/resources/windrise/license/LICENSE.txt"
        ).read_bytes()
        self.assertEqual(root_lic, webui_lic)

    def test_u15_onboarding_url_and_bridge_rebranded(self):
        hdr = read_patch("chrome/common/webui_url_constants.h")
        self.assertIn('"windrise-onboarding"', hdr)
        self.assertIn('"chrome://windrise-onboarding/"', hdr)
        self.assertNotIn("browseros-onboarding", hdr)

        cc = read_patch(
            "chrome/browser/browseros/onboarding/browseros_onboarding.cc"
        )
        self.assertIn('"windriseOnboardingPageReady"', cc)
        self.assertIn('"windriseOnboarding.receiveState"', cc)
        self.assertNotIn("browserosOnboarding", cc)

    def test_u17_no_upstream_updates_or_cdn_defaults(self):
        self.assertFalse((REPO_ROOT / "updates").exists())
        env_py = (
            REPO_ROOT / "packages/browseros/bos_build/lib/env.py"
        ).read_text(encoding="utf-8")
        self.assertNotIn("cdn.browseros.com", env_py)
        urls_ts = (
            REPO_ROOT
            / "packages/browseros-agent/packages/shared/src/constants/urls.ts"
        ).read_text(encoding="utf-8")
        self.assertNotIn("cdn.browseros.com", urls_ts)

    def test_u18_no_upstream_emails_or_issue_tracker(self):
        products_py = (
            REPO_ROOT / "packages/browseros/bos_build/core/products.py"
        ).read_text(encoding="utf-8")
        self.assertIn("https://github.com/WindriseAI/windrise-browser/issues", products_py)
        self.assertNotIn("browseros-ai/BrowserOS", products_py)

        control_in = (
            REPO_ROOT
            / "packages/browseros/bos_build/steps/package/linux_packaging/assets/debian/control.in"
        ).read_text(encoding="utf-8")
        self.assertNotIn("browseros.com", control_in)

        updater_cc = read_patch(
            "chrome/browser/browseros/server/browseros_server_updater.cc"
        )
        self.assertNotIn("nikhil@browseros.com", updater_cc)

    def test_u19_repo_root_docs_rebranded_and_license_preserved(self):
        readme = (REPO_ROOT / "README.md").read_text(encoding="utf-8")
        self.assertIn("风桥浏览器", readme)
        self.assertIn("Windrise Browser", readme)
        self.assertIn("AGPL-3.0", readme)
        self.assertNotIn("BrowserOS", readme)
        self.assertNotIn("cdn.browseros.com", readme)

        self.assertFalse((REPO_ROOT / "README.BrowserOS.md").exists())
        self.assertFalse((REPO_ROOT / "CLA.md").exists())
        self.assertFalse((REPO_ROOT / ".auctor.json").exists())
        # Legal baselines L1 & L5 must remain untouched.
        self.assertTrue((REPO_ROOT / "LICENSE").is_file())
        self.assertTrue((REPO_ROOT / "LICENSE.ungoogled_chromium").is_file())

    def test_u23_virtual_routes_only_keep_windrise_settings(self):
        hdr = read_patch("chrome/browser/browseros/core/browseros_constants.h")
        self.assertIn(
            '+    {"/settings", kWindriseAssistantExtensionId, "settings.html", ""},',
            hdr,
        )
        for dead in ('"/mcp"', '"/onboarding"', '"/claw"', '"app.html"'):
            self.assertNotIn(dead, hdr)

    def test_u20_extension_catalog_and_delegate_log_clean(self):
        hdr = read_patch("chrome/browser/browseros/core/browseros_constants.h")
        self.assertNotIn("kBrowserClawExtensionId", hdr)
        delegate = read_patch(
            "chrome/browser/extensions/chrome_extension_registrar_delegate.cc"
        )
        self.assertNotIn("BrowserOS extension", delegate)
        self.assertIn('"windrise: Extension "', delegate)


if __name__ == "__main__":
    unittest.main()
