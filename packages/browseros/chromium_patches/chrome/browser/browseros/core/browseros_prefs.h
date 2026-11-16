diff --git a/chrome/browser/browseros/core/browseros_prefs.h b/chrome/browser/browseros/core/browseros_prefs.h
new file mode 100644
index 0000000000..0cf77879a1
--- /dev/null
+++ b/chrome/browser/browseros/core/browseros_prefs.h
@@ -0,0 +1,132 @@
+// Copyright 2025 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_PREFS_H_
+#define CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_PREFS_H_
+
+#include <string>
+
+#include "components/prefs/pref_service.h"
+
+namespace user_prefs {
+class PrefRegistrySyncable;
+}  // namespace user_prefs
+
+namespace browseros {
+
+namespace prefs {
+
+// Boolean: Show labels on BrowserOS toolbar actions (default: true)
+inline constexpr char kShowToolbarLabels[] = "browseros.show_toolbar_labels";
+
+// Boolean: Enable vertical tabs (default: true)
+inline constexpr char kVerticalTabsEnabled[] =
+    "browseros.vertical_tabs_enabled";
+
+// Boolean: Start the vertical tab strip collapsed (default: false, i.e. a new
+// profile opens with the strip expanded). Once the user expands or collapses
+// it, the upstream persisted state wins.
+inline constexpr char kVerticalTabsStartCollapsed[] =
+    "browseros.vertical_tabs_start_collapsed";
+
+// String: Prefix of tab group titles that belong to an AI workspace. While the
+// active tab is in such a group, the vertical tab strip stays expanded.
+inline constexpr char kAIWorkspaceGroupPrefix[] =
+    "browseros.ai_workspace_group_prefix";
+
+// Boolean: Show saved tab groups in the bookmark bar (default: true, false for
+// BrowserClaw).
+inline constexpr char kShowTabGroupsInBookmarkBar[] =
+    "browseros.show_tab_groups_in_bookmark_bar";
+
+// Boolean: Focus NTP content instead of omnibox on new tab (default: true)
+inline constexpr char kNtpFocusContent[] = "browseros.ntp_focus_content";
+
+inline constexpr char kOnboardingCompleted[] = "browseros.onboarding_completed";
+
+// Local-only first-install records keyed by active product extension ID. An
+// empty record is eligible; discovery adds the bundled version, then READY adds
+// a fixed deadline. Expiry marks the record released until that bundled version
+// is replaced, covering installers that finish after the timer. Retaining the
+// user-set dictionary (even empty) prevents re-enrollment after expiry,
+// recovery, or a browser restart.
+inline constexpr char kExtensionInstallGrace[] =
+    "browseros.extension_install_grace";
+
+// Boolean: Automation-driven tabs never pull the user's attention. A tab counts
+// as automation-driven while a DevTools client is attached to it, which is
+// every tab the claw-server (or any CDP client) acts on. With the pref on such
+// a tab cannot switch the user's active tab or raise the window, and tabs or
+// popups its pages open land in the background. Gates live in
+// Browser::ActivateContents, Browser::AddNewContents and the DevTools
+// BrowserHandler. Default: true for BrowserClaw, false for BrowserOS.
+inline constexpr char kAutomationNeverStealsFocus[] =
+    "browseros.automation_never_steals_focus";
+
+// String: where the Windrise AI edge light  is drawn.
+//   "page"  - the Windrise extension draws it inside the page (default);
+//             the browser only animates the web-contents corners.
+//   "layer" - a transparent, input-less web view over the web contents loads
+//             the extension's edge-light/frame.html (same drawing code).
+// Any other value is treated as "page" (the native Skia drawing was removed).
+// Written by the Windrise extension through browserOS.setEdgeLightMode.
+inline constexpr char kWindriseEdgeLightMode[] =
+    "browseros.windrise.edge_light_mode";
+// String: id of the extension that last set kWindriseEdgeLightMode; the
+// "layer" web view loads chrome-extension://<id>/edge-light/frame.html.
+inline constexpr char kWindriseEdgeLightExtensionId[] =
+    "browseros.windrise.edge_light_extension_id";
+
+}  // namespace prefs
+
+// Registers BrowserOS profile preferences.
+void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
+
+// Check if toolbar labels should be shown for BrowserOS actions.
+bool ShouldShowToolbarLabels(PrefService* pref_service);
+
+// Check if vertical tabs should be enabled.
+bool IsVerticalTabsEnabled(PrefService* pref_service);
+
+// Check whether a fresh profile should start with the vertical tab strip
+// collapsed.
+bool ShouldStartVerticalTabsCollapsed(PrefService* pref_service);
+
+// Returns the configured AI workspace tab-group title prefix.
+const std::string& GetAIWorkspaceGroupPrefix(PrefService* pref_service);
+
+// Check if saved tab groups should be shown in the bookmark bar.
+bool ShouldShowTabGroupsInBookmarkBar(PrefService* pref_service);
+
+// Syncs the BrowserOS vertical tabs pref to the upstream Chrome pref.
+// Call this early (e.g. during controller init) so the upstream pref
+// reflects BrowserOS's default.
+void SyncVerticalTabsPref(PrefService* pref_service);
+
+// Applies the BrowserOS saved tab groups bookmark bar pref to the upstream
+// Chrome pref.
+void ApplyShowTabGroupsInBookmarkBarPref(PrefService* pref_service);
+
+// Syncs the BrowserOS saved tab groups bookmark bar pref to the upstream Chrome
+// pref only while the upstream pref is still at its default value.
+void SyncShowTabGroupsInBookmarkBarPref(PrefService* pref_service);
+
+// Sets the default BrowserOS theme (blue tonal spot) on first run
+// when the user hasn't customized the theme yet.
+void SyncDefaultTheme(PrefService* pref_service);
+
+// Check if a BrowserOS extension should be pinned from its catalog metadata.
+bool ShouldPinBrowserOSExtension(const std::string& extension_id,
+                                 PrefService* pref_service);
+
+// Check if NTP content should receive focus instead of the omnibox.
+bool IsNtpFocusContentEnabled(PrefService* pref_service);
+
+// Check if automation-driven tabs must never steal focus. Callers decide per
+// tab by combining this with content::DevToolsAgentHost::IsDebuggerAttached().
+bool AutomationNeverStealsFocus(PrefService* pref_service);
+
+}  // namespace browseros
+
+#endif  // CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_PREFS_H_
