"""Unit tests for #B11: Windrise settings page virtual URL route mapping.

    python packages/browseros/tools/windrise/windrise_settings_route_test.py

Verifies that browseros_constants.h maps the Windrise assistant extension's
settings.html (including any fragment such as #grids) to chrome://windrise/settings
without adding the assistant to kBrowserOSExtensions.
"""

import re
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
PATCHES = HERE.parents[1] / "chromium_patches"
ASSISTANT_ID = "amlnokjigkbgngdgkofikfphjbknfodp"


def read_patch(rel: str) -> str:
    return (PATCHES / rel).read_text(encoding="utf-8")


class WindriseSettingsRouteTest(unittest.TestCase):
    CONSTANTS = "chrome/browser/browseros/core/browseros_constants.h"
    CONTENT_CLIENT = "chrome/browser/chrome_content_browser_client.cc"
    OMNIBOX_CLIENT = "chrome/browser/ui/omnibox/chrome_omnibox_client.cc"
    BROWSER_COMMANDS = "chrome/browser/ui/browser_commands.cc"

    def test_host_uses_windrise_brand(self):
        text = read_patch(self.CONSTANTS)
        self.assertIn('+inline constexpr char kBrowserOSHost[] = "windrise";', text)

    def test_settings_route_maps_windrise_assistant_settings_html(self):
        text = read_patch(self.CONSTANTS)
        self.assertRegex(
            text,
            r'\+inline constexpr char kWindriseAssistantExtensionId\[\] =\n'
            r'\+    "%s";' % ASSISTANT_ID,
        )
        self.assertIn(
            '+    {"/settings", kWindriseAssistantExtensionId, "settings.html", ""},',
            text,
        )

    def test_assistant_stays_out_of_browseros_extension_catalog(self):
        text = read_patch(self.CONSTANTS)
        match = re.search(
            r"kBrowserOSExtensions\[\] = \{(.*?)\};",
            text,
            re.DOTALL,
        )
        self.assertIsNotNone(match, "kBrowserOSExtensions definition missing")
        catalog_body = match.group(1)
        self.assertNotIn("kWindriseAssistantExtensionId", catalog_body)
        self.assertNotIn(ASSISTANT_ID, catalog_body)

    def test_route_lookup_and_reverse_rewrite_use_route_gate(self):
        text = read_patch(self.CONSTANTS)
        self.assertIn(
            "+inline bool IsActiveURLRouteExtension(std::string_view extension_id) {",
            text,
        )
        self.assertIn(
            "+  if (extension_id == kWindriseAssistantExtensionId) {",
            text,
        )
        self.assertIn("+    return IsBrowserOSProduct();", text)
        self.assertIn(
            "+        IsActiveURLRouteExtension(route.extension_id)) {",
            text,
        )
        self.assertIn(
            "+    if (!IsActiveURLRouteExtension(route.extension_id)) {",
            text,
        )

    def test_url_handlers_and_omnibox_remain_wired(self):
        content_client = read_patch(self.CONTENT_CLIENT)
        self.assertIn("url->host() != browseros::kBrowserOSHost", content_client)
        self.assertIn("browseros::GetBrowserOSExtensionURL(url->path())", content_client)
        self.assertIn("browseros::GetBrowserOSVirtualURL(", content_client)

        omnibox = read_patch(self.OMNIBOX_CLIENT)
        self.assertGreaterEqual(
            omnibox.count("browseros::GetBrowserOSVirtualURL("), 3
        )

        commands = read_patch(self.BROWSER_COMMANDS)
        self.assertIn("browseros::GetBrowserOSVirtualURL(", commands)


if __name__ == "__main__":
    unittest.main()
