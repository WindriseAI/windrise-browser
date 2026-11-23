"""Static unit tests for DS57 browser P2 real-path fixes (BRW-012/003/004/009/007).

    python packages/browseros/tools/windrise/windrise_p2_brw_1123_test.py
"""

import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
PATCHES = HERE.parents[1] / "chromium_patches"
ASSISTANT_ID = "amlnokjigkbgngdgkofikfphjbknfodp"


def read_patch(rel: str) -> str:
    return (PATCHES / rel).read_text(encoding="utf-8")


class WindriseP2Brw1123Test(unittest.TestCase):
    def test_item1_deelevate_guard_present(self):
        delegate = read_patch("chrome/app/chrome_main_delegate.cc")
        self.assertIn("switches::kRemoteDebuggingPipe", delegate)
        self.assertIn("switches::kDoNotDeElevateOnLaunch", delegate)

    def test_item2_lease_refresh_on_stale_deadline_and_bounded_skew(self):
        protection = read_patch(
            "chrome/browser/extensions/windrise_extension_protection.cc"
        )
        # Security boundary must remain intact: 5 privileged APIs still require
        # live desktop-signed lease, capped strictly at <= 8000ms monotonic TTL.
        self.assertIn("constexpr int64_t kMaxLeaseTtlMs = 8000;", protection)
        self.assertIn("constexpr int64_t kClockSkewToleranceMs = 250;", protection)
        self.assertIn(
            "expires - now > kMaxLeaseTtlMs + kClockSkewToleranceMs", protection
        )
        self.assertIn(
            "const int64_t remaining_ms = std::min(expires - now, kMaxLeaseTtlMs);",
            protection,
        )
        self.assertIn(
            "record->lease_deadline = ticks + base::Milliseconds(remaining_ms);",
            protection,
        )

        # IsAuthorizedWindriseExtension must refresh when lease_deadline elapsed
        # even if record->authorized was still true from an earlier poll.
        auth_body = protection.split("IsAuthorizedWindriseExtension(")[1].split(
            "AddWindriseAssistantAuthorityObserver("
        )[0]
        self.assertIn("if (!record->authorized ||", auth_body)
        self.assertIn("base::TimeTicks::Now() >= record->lease_deadline)", auth_body)
        self.assertIn("RefreshWindriseAssistantAuthority(extension);", auth_body)

        api_cc = read_patch(
            "chrome/browser/extensions/api/browser_os/browser_os_api.cc"
        )
        for fn_name in (
            "WindriseSetAiTabsFunction::Run()",
            "WindriseSetConnectedAisFunction::Run()",
            "WindriseSetAiActivityFunction::Run()",
            "WindriseSetEdgeLightModeFunction::Run()",
            "WindrisePulseAiActivityFunction::Run()",
        ):
            snippet = api_cc.split(fn_name)[1][:220]
            self.assertIn("if (!IsWindriseCaller(extension()))", snippet)

    def test_item3_rail_glyph_rules_and_default_tab_fallback(self):
        glyph_cc = read_patch("chrome/browser/browseros/core/windrise_rail_glyph.cc")
        # 1. Chinese Han first, 2. Latin uppercase, 3. first visible char, 4. host IDN.
        self.assertIn("ScriptOf(it.get()) == USCRIPT_HAN", glyph_cc)
        self.assertIn("u_isalpha(c) && ScriptOf(c) == USCRIPT_LATIN", glyph_cc)
        self.assertIn("FromCodePoint(u_toupper(c))", glyph_cc)
        self.assertIn("IsVisible(it.get())", glyph_cc)
        self.assertIn('host.starts_with("www.")', glyph_cc)
        self.assertIn("UnsafeIDNToUnicodeWithDetails(host)", glyph_cc)

        tab_view = read_patch("chrome/browser/ui/views/tabs/common/tab_view.cc")
        self.assertIn("rail_glyph_ = AddChildView(std::make_unique<views::Label>());", tab_view)
        self.assertIn("UpdateRailGlyph();", tab_view)
        # Untitled / about:blank / data: / local files without host fall back
        # to CoreTabHelper::GetDefaultTitle() when not showing loading animation.
        update_glyph = tab_view.split("void TabView::UpdateRailGlyph()")[1].split(
            "void TabView::UpdateRailGlyphFont()"
        )[0]
        self.assertIn("!icon_->GetShowingLoadingAnimation()", update_glyph)
        self.assertIn("CoreTabHelper::GetDefaultTitle()", update_glyph)
        self.assertIn("rail_glyph_->SetText(glyph);", update_glyph)
        self.assertIn("InvalidateLayout();", update_glyph)

        layout = read_patch(
            "chrome/browser/ui/views/tabs/common/tab_view_vertical_layout.cc"
        )
        self.assertIn("const bool show_glyph = centered_index.has_value() &&", layout)
        self.assertIn("!TabView().pinned_ && !glyph->GetText().empty() &&", layout)
        self.assertIn("width >= group_width;", layout)
        self.assertIn("layouts.child_layouts.emplace_back(\n+          glyph, true,", layout)

    def test_item4_ai_panel_buttons_and_gallery_u039_toast(self):
        panel_cc = read_patch(
            "chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc"
        )
        build_gn = read_patch("chrome/browser/ui/views/tabs/vertical/BUILD.gn")

        # Gallery middle button must NOT be silently disabled; stays clickable
        # with muted styling, coming-soon caption, event broadcast, and toast.
        self.assertNotIn("gallery->SetEnabled(false);", panel_cc)
        self.assertIn("IDS_WINDRISE_AI_GALLERY_SOON", panel_cc)
        self.assertIn("ToastController::From(browser)", panel_cc)
        self.assertIn("ToastId::kBrowserOSToast", panel_cc)

        # Settings / open_app / connect_ai broadcast onAiPanelAction AND have
        # reliable native fallback + close bubble widget cleanly.
        notify_body = panel_cc.split("void WindriseAiPanelView::NotifyAction(")[1]
        self.assertIn('kPanelActionEventName', notify_body)
        self.assertIn("event_router->BroadcastEvent(", notify_body)
        self.assertIn("if (action_view == kActionGallery)", notify_body)
        self.assertIn("else if (action_view == kActionSettings)", notify_body)
        self.assertIn("action_view == kActionOpenApp", notify_body)
        self.assertIn("action_view == kActionConnectAi", notify_body)
        self.assertIn("IsAuthorizedWindriseExtension(assistant)", notify_body)
        self.assertIn("ShowSingletonTab(browser,", notify_body)
        self.assertIn("kWindriseSettingsFallbackUrl", notify_body)
        self.assertIn("chrome::kChromeUISettingsURL", notify_body)
        self.assertIn("widget->Close();", notify_body)

        # GN deps for windrise_ai_ui must include singleton_tabs, toasts, core, common, url.
        ai_ui_block = build_gn.split('source_set("windrise_ai_ui")')[1]
        for dep in (
            '"//chrome/browser/browseros/core"',
            '"//chrome/browser/ui:singleton_tabs"',
            '"//chrome/browser/ui/toasts"',
            '"//chrome/browser/ui/toasts/api:toasts"',
            '"//chrome/common"',
            '"//url"',
        ):
            self.assertIn(dep, ai_ui_block)


if __name__ == "__main__":
    unittest.main()
