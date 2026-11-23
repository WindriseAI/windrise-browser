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
    def test_item1_deelevate_boundary_restored(self):
        delegate_path = PATCHES / "chrome/app/chrome_main_delegate.cc"
        self.assertFalse(
            delegate_path.exists(),
            "upstream MaybeAutoDeElevate must not exempt --remote-debugging-pipe",
        )
        features_yaml = (PATCHES / ".features.yaml").read_text(encoding="utf-8")
        self.assertNotIn("chrome/app/chrome_main_delegate.cc", features_yaml)

    def test_item2_lease_system_clock_strict_ttl_and_refresh(self):
        protection = read_patch(
            "chrome/browser/extensions/windrise_extension_protection.cc"
        )
        # Security boundary must remain intact: 5 privileged APIs still require
        # live desktop-signed lease, checked against system wall clock with
        # strict 8000ms TTL ceiling (no skew tolerance expansion).
        self.assertIn("constexpr int64_t kMaxLeaseTtlMs = 8000;", protection)
        self.assertNotIn("kClockSkewToleranceMs", protection)
        self.assertIn(
            "base::Time::NowFromSystemTime().InMillisecondsSinceUnixEpoch();",
            protection,
        )
        self.assertNotIn(
            "base::Time::Now().InMillisecondsSinceUnixEpoch()", protection
        )
        self.assertIn("expires <= now || expires - now > kMaxLeaseTtlMs)", protection)
        self.assertIn(
            "record->lease_deadline = ticks + base::Milliseconds(expires - now);",
            protection,
        )
        self.assertIn("record->expired_lease = record->last_lease;", protection)
        self.assertIn("!record->expired_lease.empty() && lease == record->expired_lease", protection)
        # Existing security checks must remain intact.
        self.assertIn('constexpr char kAssistantId[] = "' + ASSISTANT_ID + '";', protection)
        self.assertIn("lines[1] != kAssistantId || lines[2] != record->digest", protection)
        self.assertIn(
            "crypto::sign::Verify(crypto::sign::RSA_PKCS1_SHA256, *LeaseKey(),",
            protection,
        )
        self.assertIn("VerifySignedDirectory(record->extension->path(), &digest)", protection)
        self.assertIn("record->needs_reverify = true;", protection)
        self.assertIn("base::FilePathWatcher::RecursiveWatchAvailable()", protection)

        # IsAuthorizedWindriseExtension must refresh when lease_deadline elapsed
        # or when system wall clock reaches lease_expires_ms.
        auth_body = protection.split("IsAuthorizedWindriseExtension(")[1].split(
            "AddWindriseAssistantAuthorityObserver("
        )[0]
        self.assertIn("if (!record->authorized ||", auth_body)
        self.assertIn("ticks >= record->lease_deadline ||", auth_body)
        self.assertIn("now_ms >= record->lease_expires_ms ||", auth_body)
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

    def test_item2_lease_state_machine_expiry_renewal_drift_refresh_and_revocation(self):
        """Model exact HasLiveLease/PollAuthority/IsAuthorizedWindriseExtension semantics."""
        MAX_TTL_MS = 8000

        class LeaseVerifier:
            def __init__(self, digest="abc123", valid=True):
                self.valid = valid
                self.authorized = False
                self.needs_reverify = False
                self.digest = digest
                self.last_lease = ""
                self.expired_lease = ""
                self.last_size = -1
                self.last_mtime = None
                self.lease_expires_ms = 0
                self.lease_deadline = 0
                self.reverify_count = 0
                self.reverify_ok = True

            def _clear(self):
                self.last_lease = ""
                self.last_size = -1
                self.last_mtime = None
                self.lease_expires_ms = 0
                self.lease_deadline = 0

            def _is_cached_live(self, now_system_ms, ticks_ms):
                if (
                    ticks_ms < self.lease_deadline
                    and now_system_ms < self.lease_expires_ms
                    and self.lease_expires_ms - now_system_ms <= MAX_TTL_MS
                ):
                    return True
                self.expired_lease = self.last_lease
                self._clear()
                return False

            def has_live_lease(self, lease_file, now_system_ms, ticks_ms):
                if not self.valid or lease_file is None:
                    self._clear()
                    return False
                content, mtime = lease_file
                size = len(content)
                if self.last_lease and size == self.last_size and mtime == self.last_mtime:
                    return self._is_cached_live(now_system_ms, ticks_ms)
                if content == self.last_lease:
                    self.last_size = size
                    self.last_mtime = mtime
                    return self._is_cached_live(now_system_ms, ticks_ms)
                if self.expired_lease and content == self.expired_lease:
                    self._clear()
                    return False
                lines = content.split("\n")
                if len(lines) != 6 or lines[5] != "":
                    self._clear()
                    return False
                if lines[0] != "windrise-assistant-lease-v1" or lines[1] != ASSISTANT_ID or lines[2] != self.digest:
                    self._clear()
                    return False
                try:
                    expires = int(lines[3])
                except ValueError:
                    self._clear()
                    return False
                if expires <= now_system_ms or expires - now_system_ms > MAX_TTL_MS:
                    self._clear()
                    return False
                if lines[4] != "VALID_SIG":
                    self._clear()
                    return False
                self.last_lease = content
                self.last_size = size
                self.last_mtime = mtime
                self.lease_expires_ms = expires
                self.lease_deadline = ticks_ms + (expires - now_system_ms)
                return True

            def poll(self, lease_file, now_system_ms, ticks_ms):
                authorized = self.has_live_lease(lease_file, now_system_ms, ticks_ms)
                if authorized and not self.authorized and self.needs_reverify:
                    self.reverify_count += 1
                    if not self.reverify_ok:
                        self.valid = False
                        authorized = False
                    else:
                        self.needs_reverify = False
                elif not authorized and self.authorized:
                    self.needs_reverify = True
                self.authorized = authorized

            def is_authorized(self, lease_file, now_system_ms, ticks_ms):
                if not self.valid:
                    return False
                if (
                    not self.authorized
                    or ticks_ms >= self.lease_deadline
                    or self.lease_expires_ms <= now_system_ms
                    or self.lease_expires_ms - now_system_ms > MAX_TTL_MS
                ):
                    self.poll(lease_file, now_system_ms, ticks_ms)
                return (
                    self.authorized
                    and ticks_ms < self.lease_deadline
                    and self.lease_expires_ms > now_system_ms
                    and self.lease_expires_ms - now_system_ms <= MAX_TTL_MS
                )

        def make_lease(expires_ms, sig="VALID_SIG", digest="abc123", ext_id=ASSISTANT_ID):
            return f"windrise-assistant-lease-v1\n{ext_id}\n{digest}\n{expires_ms}\n{sig}\n"

        v = LeaseVerifier()

        # 1. Fresh renewal: signed with desktop Date.now() + 8000ms -> accepted
        fresh = (make_lease(108_000), 1)
        self.assertTrue(v.is_authorized(fresh, now_system_ms=100_000, ticks_ms=1_000))
        self.assertEqual(v.lease_deadline, 9_000)

        # 2. Exceeds strict 8000ms ceiling (e.g. 8001ms / 8250ms) -> rejected
        v_over = LeaseVerifier()
        self.assertFalse(
            v_over.is_authorized((make_lease(108_001), 1), now_system_ms=100_000, ticks_ms=1_000)
        )
        self.assertFalse(
            v_over.is_authorized((make_lease(108_250), 2), now_system_ms=100_000, ticks_ms=1_000)
        )

        # 3. Expired lease (system clock past signed expiry, even if an interpolated
        #    QPC clock lagged by 200ms behind system time) -> rejected
        v_drift = LeaseVerifier()
        self.assertFalse(
            v_drift.is_authorized((make_lease(100_000), 1), now_system_ms=100_000, ticks_ms=1_000)
        )
        self.assertFalse(
            v_drift.is_authorized((make_lease(99_950), 2), now_system_ms=100_000, ticks_ms=1_000)
        )

        # 4. Monotonic deadline expiry triggers active refresh:
        #    - stale file on disk stays rejected, marks needs_reverify, and clears cached fields
        self.assertFalse(v.is_authorized(fresh, now_system_ms=108_000, ticks_ms=9_000))
        self.assertTrue(v.needs_reverify)
        self.assertEqual(v.last_lease, "")
        self.assertEqual(v.lease_deadline, 0)
        self.assertEqual(v.lease_expires_ms, 0)

        # 4b. Cached hit when system wall clock jumps forward past signed expiry
        #     before monotonic deadline is reached -> MUST reject and clear cache!
        v_jump = LeaseVerifier()
        self.assertTrue(v_jump.is_authorized(fresh, now_system_ms=100_000, ticks_ms=1_000))
        self.assertFalse(v_jump.is_authorized(fresh, now_system_ms=108_001, ticks_ms=2_000))
        self.assertEqual(v_jump.last_lease, "")
        self.assertEqual(v_jump.lease_deadline, 0)

        # 4c. Clock rollback after lease expired MUST NOT resurrect the same expired lease!
        self.assertFalse(v_jump.is_authorized(fresh, now_system_ms=102_000, ticks_ms=2_500))
        self.assertFalse(v.is_authorized(fresh, now_system_ms=102_000, ticks_ms=9_500))
        self.assertEqual(v.expired_lease, fresh[0])

        #    - renewed file on disk restores authority and forces directory reverify
        renewed = (make_lease(116_000), 2)
        self.assertTrue(v.is_authorized(renewed, now_system_ms=108_000, ticks_ms=9_000))
        self.assertEqual(v.reverify_count, 1)
        self.assertFalse(v.needs_reverify)

        # 5. Revocation (expires=0) immediately revokes and clears cached deadline;
        #    tampered directory during post-revocation recovery permanently fails.
        revoked = (make_lease(0), 3)
        v.poll(revoked, now_system_ms=109_000, ticks_ms=10_000)
        self.assertFalse(v.authorized)
        self.assertTrue(v.needs_reverify)
        self.assertEqual(v.lease_deadline, 0)
        v.reverify_ok = False
        reissued = (make_lease(117_000), 4)
        self.assertFalse(v.is_authorized(reissued, now_system_ms=109_500, ticks_ms=10_500))
        self.assertEqual(v.reverify_count, 2)
        self.assertFalse(v.valid)

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
