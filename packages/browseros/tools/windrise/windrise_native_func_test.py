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
        self.assertIn("if (!IsVerifiedWindriseCaller(extension()))", fn_body)
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
        self.assertIn("if (!IsVerifiedWindriseCaller(extension()))", fn_body)
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
        self.assertIn("if (!IsVerifiedWindriseCaller(extension()))", get_body)
        self.assertIn("ThemeServiceFactory::GetForProfile(profile)", get_body)
        self.assertIn("GetBrowserColorScheme()", get_body)
        self.assertIn("BrowserUsesDarkColors()", get_body)

        set_body = api_cc.split("WindriseSetColorSchemeFunction::Run()")[1]
        self.assertIn("if (!IsVerifiedWindriseCaller(extension()))", set_body)
        self.assertIn('params->mode == "system"', set_body)
        self.assertIn('params->mode == "light"', set_body)
        self.assertIn('params->mode == "dark"', set_body)
        self.assertIn("SetBrowserColorScheme(scheme)", set_body)

    def test_p0_security_allowlist_and_pref_whitelist(self):
        import hashlib

        expected_sha1 = hashlib.sha1(ASSISTANT_ID.encode("ascii")).hexdigest().upper()
        perm_features = read_patch(
            "chrome/common/extensions/api/_permission_features.json"
        )
        self.assertIn('"allowlist": [', perm_features)
        self.assertIn(f'"{expected_sha1}"', perm_features)

        api_cc = read_patch(
            "chrome/browser/extensions/api/browser_os/browser_os_api.cc"
        )
        self.assertIn("bool IsReadableWindrisePref(", api_cc)
        self.assertIn('pref_name.starts_with("windrise.")', api_cc)
        self.assertIn('pref_name.starts_with("browser.theme.")', api_cc)
        self.assertIn('pref_name == "browser.color_scheme"', api_cc)

        for fn_name in (
            "WindriseGetPrefFunction::Run()",
            "WindriseSetPrefFunction::Run()",
            "WindriseChoosePathFunction::Run()",
            "WindriseShowToastFunction::Run()",
            "WindriseShowInfoBarFunction::Run()",
            "WindriseSetAddressBarSlotFunction::Run()",
            "WindriseGetDefaultSearchEngineFunction::Run()",
            "WindriseGetVersionNumberFunction::Run()",
            "WindriseGetWindriseVersionNumberFunction::Run()",
        ):
            snippet = api_cc.split(fn_name)[1][:300]
            self.assertIn("if (!IsVerifiedWindriseCaller(extension()))", snippet)

        for lease_fn in (
            "WindriseSetAiTabsFunction::Run()",
            "WindriseSetConnectedAisFunction::Run()",
            "WindriseSetAiActivityFunction::Run()",
            "WindriseSetEdgeLightModeFunction::Run()",
            "WindrisePulseAiActivityFunction::Run()",
        ):
            snippet = api_cc.split(lease_fn)[1][:200]
            self.assertIn("if (!IsWindriseCaller(extension()))", snippet)

    def test_incognito_120hz_and_protection_cleanup(self):
        mixer = read_patch("chrome/browser/ui/color/windrise_color_mixer.cc")
        self.assertIn("GetIncognitoColors()", mixer)
        self.assertIn("UserColorSource::kGrayscale", mixer)
        # Chromium derives the generic WebContents background from the NTP token.
        self.assertNotIn("kColorNewTabPageBackground", mixer)
        self.assertNotIn("kColorWebContentsBackground", mixer)

        contents_view = read_patch("chrome/browser/ui/views/frame/contents_web_view.cc")
        self.assertIn("IsOffTheRecord()", contents_view)

        card_cc = read_patch(
            "chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.cc"
        )
        self.assertNotIn("kDefaultFrameRate", card_cc)
        self.assertIn("kContentFadeFrameRate = 120", card_cc)
        self.assertIn("RichAnimationDuration(kContentFadeDuration)", card_cc)

        protection = read_patch(
            "chrome/browser/extensions/windrise_extension_protection.cc"
        )
        self.assertIn("OnExtensionUnloaded(", protection)
        self.assertIn("OnShutdown(ExtensionRegistry* registry)", protection)
        self.assertIn("EraseVerification(", protection)
        self.assertIn("base::Milliseconds(1000)", protection)
        self.assertIn("LeaseKey().has_value()", protection)

        shared_vars = read_patch("ui/webui/resources/cr_elements/cr_shared_vars.css")
        self.assertNotIn("fixed !important", shared_vars)
        self.assertIn("body::before", shared_vars)
        self.assertIn("color-scheme: light dark;", shared_vars)
        self.assertIn("html:not(#wr-tokens)[dark]", shared_vars)
        self.assertIn("@media (prefers-color-scheme: dark)", shared_vars)

        settings_sec = read_patch(
            "chrome/browser/resources/settings/settings_page/settings_section.css"
        )
        self.assertIn("var(--wr-card,", settings_sec)
        self.assertIn("var(--wr-card-shadow,", settings_sec)

        neterror = read_patch("components/neterror/resources/neterror.css")
        self.assertNotIn("background-attachment: fixed", neterror)
        self.assertIn("@media (prefers-color-scheme: dark)", neterror)

        for zh_xtb in (
            "chrome/app/resources/chromium_strings_zh-CN.xtb",
            "chrome/app/resources/chromium_strings_zh-HK.xtb",
            "chrome/app/resources/chromium_strings_zh-TW.xtb",
            "chrome/app/resources/generated_resources_zh-CN.xtb",
            "chrome/app/resources/generated_resources_zh-HK.xtb",
            "chrome/app/resources/generated_resources_zh-TW.xtb",
        ):
            added = [
                line for line in read_patch(zh_xtb).splitlines() if line.startswith("+")
            ]
            self.assertFalse(
                any("Windrise Browser" in line for line in added),
                f"unexpected English brand in Chinese xtb: {zh_xtb}",
            )
            self.assertTrue(
                any("风桥浏览器" in line for line in added),
                f"missing Chinese brand in Chinese xtb: {zh_xtb}",
            )

    def test_brw001_zh_cn_product_name_for_shortcut_and_profile_picker(self):
        # 5906362466875665739 is tclib.GenerateMessageId("Windrise Browser"),
        # used by both Windows installer shortcuts (IDS_PRODUCT_NAME_BASE via
        # create_installer_string_rc) and ProfilePickerView::SetTitle(IDS_PRODUCT_NAME).
        expected_entry = '+<translation id="5906362466875665739">风桥浏览器</translation>'
        for zh_xtb in (
            "chrome/app/resources/chromium_strings_zh-CN.xtb",
            "chrome/app/resources/chromium_strings_zh-HK.xtb",
            "chrome/app/resources/chromium_strings_zh-TW.xtb",
        ):
            self.assertIn(expected_entry, read_patch(zh_xtb))

        string_replaces_py = (
            HERE.parents[1] / "bos_build/steps/resources/string_replaces.py"
        ).read_text(encoding="utf-8")
        self.assertIn('IDS_(?:SHORT_)?PRODUCT_NAME', string_replaces_py)
        self.assertIn('translateable="true"', string_replaces_py)

        # ProfilePickerView must keep upstream kWindowTitleId (IDS_PRODUCT_NAME)
        # without hardcoding "Windrise Browser" in GetWindowTitle / SetTitle.
        picker_cc = read_patch(
            "chrome/browser/ui/views/profiles/profile_picker_view.cc"
        )
        self.assertNotIn("Windrise Browser", picker_cc)
        self.assertNotIn("GetWindowTitle(", picker_cc)

        # Installer patches must not override InstallUtil::GetShortcutName with
        # a hardcoded "Windrise Browser" shortcut name.
        for inst_rel in (
            "chrome/installer/setup/setup_main.cc",
            "chrome/installer/setup/browseros_install_ui.cc",
        ):
            self.assertNotIn("set_shortcut_name", read_patch(inst_rel))


if __name__ == "__main__":
    unittest.main()
