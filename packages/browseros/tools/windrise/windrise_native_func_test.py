"""Unit tests for Windrise native browser function tail items (#B11, #B12, #B13, #B20).

    python packages/browseros/tools/windrise/windrise_native_func_test.py
"""

import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
PATCHES = HERE.parents[1] / "chromium_patches"
ASSISTANT_ID = "amlnokjigkbgngdgkofikfphjbknfodp"


def read_patch(rel: str) -> str:
    return (PATCHES / rel).read_text(encoding="utf-8")


class WindriseNativeFuncTest(unittest.TestCase):
    def test_b11_assistant_id_consistent_across_native_patches(self):
        constants = read_patch("chrome/browser/browseros/core/browseros_constants.h")
        protection = read_patch(
            "chrome/browser/extensions/windrise_extension_protection.cc"
        )
        contents_view = read_patch(
            "chrome/browser/ui/views/frame/contents_web_view.cc"
        )
        self.assertIn(f'"{ASSISTANT_ID}"', constants)
        self.assertIn(f'constexpr char kAssistantId[] = "{ASSISTANT_ID}";', protection)
        self.assertIn(f'"{ASSISTANT_ID}";', contents_view)

    def test_b11_forward_route_preserves_fragment_and_skips_ntp_dialog(self):
        client = read_patch("chrome/browser/chrome_content_browser_client.cc")
        self.assertIn("url->has_ref() && !url->ref().empty()", client)
        self.assertIn("extension_url.push_back('#');", client)
        self.assertIn("extension_url.append(url->ref());", client)

        overridden = read_patch(
            "chrome/browser/ui/extensions/settings_overridden_params_providers.cc"
        )
        self.assertIn("browseros::IsActiveURLRouteExtension(extension->id())", overridden)

    def test_b12_set_default_search_engine_whitelisted_and_gated(self):
        idl = read_patch("chrome/common/extensions/api/windrise.idl")
        api_h = read_patch(
            "chrome/browser/extensions/api/browser_os/browser_os_api.h"
        )
        api_cc = read_patch(
            "chrome/browser/extensions/api/browser_os/browser_os_api.cc"
        )
        hist = read_patch(
            "extensions/browser/extension_function_histogram_value.h"
        )
        enums = read_patch("tools/metrics/histograms/metadata/extensions/enums.xml")

        self.assertIn("static void setDefaultSearchEngine(", idl)
        self.assertNotIn("browseros", idl.lower())
        self.assertIn(
            'DECLARE_EXTENSION_FUNCTION("windrise.setDefaultSearchEngine",', api_h
        )
        self.assertIn("WINDRISE_SETDEFAULTSEARCHENGINE", api_h)
        self.assertIn("WINDRISE_SETDEFAULTSEARCHENGINE = 2046,", hist)
        self.assertIn(
            '<int value="2046" label="WINDRISE_SETDEFAULTSEARCHENGINE"/>', enums
        )

        fn_body = api_cc.split("WindriseSetDefaultSearchEngineFunction::Run()")[1]
        fn_body = fn_body.split("WindriseSetNewTabAsHomePageFunction::Run()")[0]
        self.assertIn("if (!IsWindriseCaller(extension()))", fn_body)
        self.assertIn('params->provider_id == "google"', fn_body)
        self.assertIn("TemplateURLPrepopulateData::google.id", fn_body)
        self.assertIn('params->provider_id == "baidu"', fn_body)
        self.assertIn("TemplateURLPrepopulateData::baidu.id", fn_body)
        self.assertIn("CanDefaultSearchProviderBeModifiedByUser()", fn_body)
        self.assertIn(
            "SetUserSelectedDefaultSearchProviderByPrepopulateId(prepopulate_id_)",
            fn_body,
        )

    def test_b13_set_new_tab_as_home_page_safe_and_gated(self):
        idl = read_patch("chrome/common/extensions/api/windrise.idl")
        api_h = read_patch(
            "chrome/browser/extensions/api/browser_os/browser_os_api.h"
        )
        api_cc = read_patch(
            "chrome/browser/extensions/api/browser_os/browser_os_api.cc"
        )
        hist = read_patch(
            "extensions/browser/extension_function_histogram_value.h"
        )
        enums = read_patch("tools/metrics/histograms/metadata/extensions/enums.xml")

        self.assertIn("static void setNewTabAsHomePage(SetPrefCallback callback);", idl)
        self.assertIn(
            'DECLARE_EXTENSION_FUNCTION("windrise.setNewTabAsHomePage",', api_h
        )
        self.assertIn("WINDRISE_SETNEWTABASHOMEPAGE = 2047,", hist)
        self.assertIn(
            '<int value="2047" label="WINDRISE_SETNEWTABASHOMEPAGE"/>', enums
        )

        fn_body = api_cc.split("WindriseSetNewTabAsHomePageFunction::Run()")[1]
        fn_body = fn_body.split("WindriseGetColorSchemeFunction::Run()")[0]
        self.assertIn("if (!IsWindriseCaller(extension()))", fn_body)
        self.assertIn("SessionStartupPref::TypeIsManaged(prefs)", fn_body)
        self.assertIn("SessionStartupPref::DEFAULT", fn_body)
        self.assertIn("SessionStartupPref::SetStartupPref(prefs, startup_pref)", fn_body)
        self.assertIn("::prefs::kHomePageIsNewTabPage", fn_body)

    def test_b20_color_scheme_get_and_set_gated(self):
        idl = read_patch("chrome/common/extensions/api/windrise.idl")
        api_h = read_patch(
            "chrome/browser/extensions/api/browser_os/browser_os_api.h"
        )
        api_cc = read_patch(
            "chrome/browser/extensions/api/browser_os/browser_os_api.cc"
        )
        hist = read_patch(
            "extensions/browser/extension_function_histogram_value.h"
        )
        enums = read_patch("tools/metrics/histograms/metadata/extensions/enums.xml")

        self.assertIn("dictionary ColorSchemeState {", idl)
        self.assertIn("static void getColorScheme(GetColorSchemeCallback callback);", idl)
        self.assertIn("static void setColorScheme(", idl)
        self.assertIn('DECLARE_EXTENSION_FUNCTION("windrise.getColorScheme",', api_h)
        self.assertIn('DECLARE_EXTENSION_FUNCTION("windrise.setColorScheme",', api_h)
        self.assertIn("WINDRISE_GETCOLORSCHEME = 2048,", hist)
        self.assertIn("WINDRISE_SETCOLORSCHEME = 2049,", hist)
        self.assertIn('<int value="2048" label="WINDRISE_GETCOLORSCHEME"/>', enums)
        self.assertIn('<int value="2049" label="WINDRISE_SETCOLORSCHEME"/>', enums)

        get_body = api_cc.split("WindriseGetColorSchemeFunction::Run()")[1]
        get_body = get_body.split("WindriseSetColorSchemeFunction::Run()")[0]
        self.assertIn("if (!IsWindriseCaller(extension()))", get_body)
        self.assertIn("ThemeServiceFactory::GetForProfile(profile)", get_body)
        self.assertIn("GetBrowserColorScheme()", get_body)
        self.assertIn("BrowserUsesDarkColors()", get_body)

        set_body = api_cc.split("WindriseSetColorSchemeFunction::Run()")[1]
        self.assertIn("if (!IsWindriseCaller(extension()))", set_body)
        self.assertIn('params->mode == "system"', set_body)
        self.assertIn('params->mode == "light"', set_body)
        self.assertIn('params->mode == "dark"', set_body)
        self.assertIn("SetBrowserColorScheme(scheme)", set_body)


if __name__ == "__main__":
    unittest.main()
