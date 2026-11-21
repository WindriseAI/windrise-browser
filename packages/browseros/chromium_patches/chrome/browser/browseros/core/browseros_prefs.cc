diff --git a/chrome/browser/browseros/core/browseros_prefs.cc b/chrome/browser/browseros/core/browseros_prefs.cc
new file mode 100644
index 0000000000..78c608a1db
--- /dev/null
+++ b/chrome/browser/browseros/core/browseros_prefs.cc
@@ -0,0 +1,205 @@
+// Copyright 2025 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/core/browseros_prefs.h"
+
+#include "chrome/browser/browseros/core/browseros_constants.h"
+#include "chrome/browser/browseros/core/browseros_product.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/common/pref_names.h"
+#include "components/bookmarks/common/bookmark_pref_names.h"
+#include "components/pref_registry/pref_registry_syncable.h"
+#include "base/values.h"
+#include "third_party/skia/include/core/SkColor.h"
+#include "ui/base/mojom/themes.mojom.h"
+
+namespace browseros {
+
+namespace {
+
+constexpr char kLegacyShowToolbarLabels[] = "browseros.show_toolbar_labels";
+constexpr char kLegacyVerticalTabsEnabled[] = "browseros.vertical_tabs_enabled";
+constexpr char kLegacyVerticalTabsStartCollapsed[] =
+    "browseros.vertical_tabs_start_collapsed";
+constexpr char kLegacyAIWorkspaceGroupPrefix[] =
+    "browseros.ai_workspace_group_prefix";
+constexpr char kLegacyShowTabGroupsInBookmarkBar[] =
+    "browseros.show_tab_groups_in_bookmark_bar";
+constexpr char kLegacyNtpFocusContent[] = "browseros.ntp_focus_content";
+constexpr char kLegacyOnboardingCompleted[] = "browseros.onboarding_completed";
+constexpr char kLegacyExtensionInstallGrace[] =
+    "browseros.extension_install_grace";
+constexpr char kLegacyAutomationNeverStealsFocus[] =
+    "browseros.automation_never_steals_focus";
+constexpr char kLegacyWindriseEdgeLightMode[] =
+    "browseros.windrise.edge_light_mode";
+constexpr char kLegacyWindriseEdgeLightExtensionId[] =
+    "browseros.windrise.edge_light_extension_id";
+
+void MigratePrefValue(PrefService* pref_service,
+                      const char* old_key,
+                      const char* new_key) {
+  if (!pref_service || !pref_service->FindPreference(old_key) ||
+      !pref_service->FindPreference(new_key)) {
+    return;
+  }
+  if (const base::Value* old_value = pref_service->GetUserPrefValue(old_key)) {
+    if (!pref_service->GetUserPrefValue(new_key)) {
+      pref_service->Set(new_key, old_value->Clone());
+    }
+    pref_service->ClearPref(old_key);
+  }
+}
+
+void RegisterLegacyProfilePrefsForMigration(
+    user_prefs::PrefRegistrySyncable* registry) {
+  registry->RegisterBooleanPref(kLegacyShowToolbarLabels,
+                                !IsBrowserClawProduct());
+  registry->RegisterBooleanPref(kLegacyVerticalTabsEnabled, true);
+  registry->RegisterBooleanPref(kLegacyVerticalTabsStartCollapsed, false);
+  registry->RegisterStringPref(kLegacyAIWorkspaceGroupPrefix, "AI ");
+  registry->RegisterBooleanPref(kLegacyShowTabGroupsInBookmarkBar, false);
+  registry->RegisterBooleanPref(kLegacyNtpFocusContent, false);
+  registry->RegisterBooleanPref(kLegacyOnboardingCompleted, false);
+  registry->RegisterDictionaryPref(kLegacyExtensionInstallGrace);
+  registry->RegisterBooleanPref(kLegacyAutomationNeverStealsFocus,
+                                IsBrowserClawProduct());
+  registry->RegisterStringPref(kLegacyWindriseEdgeLightMode, "page");
+  registry->RegisterStringPref(kLegacyWindriseEdgeLightExtensionId, "");
+}
+
+}  // namespace
+
+void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
+  registry->RegisterBooleanPref(prefs::kShowToolbarLabels,
+                                !IsBrowserClawProduct());
+  registry->RegisterBooleanPref(prefs::kVerticalTabsEnabled, true);
+  // 风桥竖向标签栏首启即展开（帧里都是展开态）。
+  registry->RegisterBooleanPref(prefs::kVerticalTabsStartCollapsed, false);
+  registry->RegisterStringPref(prefs::kAIWorkspaceGroupPrefix, "AI ");
+  registry->RegisterBooleanPref(prefs::kShowTabGroupsInBookmarkBar, false);
+
+  registry->RegisterBooleanPref(prefs::kNtpFocusContent, false);
+  registry->RegisterBooleanPref(prefs::kOnboardingCompleted, false);
+  registry->RegisterDictionaryPref(prefs::kExtensionInstallGrace);
+  // BrowserClaw is a browser for agents: they work in the background by
+  // default. BrowserOS keeps stock focus behaviour.
+  registry->RegisterBooleanPref(prefs::kAutomationNeverStealsFocus,
+                                IsBrowserClawProduct());
+  // 风桥 光效画在哪（page / layer），扩展经 setEdgeLightMode 写。
+  registry->RegisterStringPref(prefs::kWindriseEdgeLightMode, "page");
+  registry->RegisterStringPref(prefs::kWindriseEdgeLightExtensionId, "");
+
+  RegisterLegacyProfilePrefsForMigration(registry);
+
+  // 风桥外观数值（颜色、圆角、尺寸）：偏好即外观，运行时改不用重编。
+  windrise::RegisterProfilePrefs(registry);
+}
+
+void MigrateProfilePrefs(PrefService* pref_service) {
+  if (!pref_service) {
+    return;
+  }
+  MigratePrefValue(pref_service, kLegacyShowToolbarLabels,
+                   prefs::kShowToolbarLabels);
+  MigratePrefValue(pref_service, kLegacyVerticalTabsEnabled,
+                   prefs::kVerticalTabsEnabled);
+  MigratePrefValue(pref_service, kLegacyVerticalTabsStartCollapsed,
+                   prefs::kVerticalTabsStartCollapsed);
+  MigratePrefValue(pref_service, kLegacyAIWorkspaceGroupPrefix,
+                   prefs::kAIWorkspaceGroupPrefix);
+  MigratePrefValue(pref_service, kLegacyShowTabGroupsInBookmarkBar,
+                   prefs::kShowTabGroupsInBookmarkBar);
+  MigratePrefValue(pref_service, kLegacyNtpFocusContent,
+                   prefs::kNtpFocusContent);
+  MigratePrefValue(pref_service, kLegacyOnboardingCompleted,
+                   prefs::kOnboardingCompleted);
+  MigratePrefValue(pref_service, kLegacyExtensionInstallGrace,
+                   prefs::kExtensionInstallGrace);
+  MigratePrefValue(pref_service, kLegacyAutomationNeverStealsFocus,
+                   prefs::kAutomationNeverStealsFocus);
+  MigratePrefValue(pref_service, kLegacyWindriseEdgeLightMode,
+                   prefs::kWindriseEdgeLightMode);
+  MigratePrefValue(pref_service, kLegacyWindriseEdgeLightExtensionId,
+                   prefs::kWindriseEdgeLightExtensionId);
+  MigratePrefValue(pref_service, "browseros.third_party_llm.providers",
+                   "windrise.third_party_llm.providers");
+  MigratePrefValue(pref_service, "browseros.third_party_llm.selected_provider",
+                   "windrise.third_party_llm.selected_provider");
+  windrise::MigrateProfilePrefs(pref_service);
+}
+
+bool ShouldShowToolbarLabels(PrefService* pref_service) {
+  return pref_service->GetBoolean(prefs::kShowToolbarLabels);
+}
+
+bool IsVerticalTabsEnabled(PrefService* pref_service) {
+  return pref_service->GetBoolean(prefs::kVerticalTabsEnabled);
+}
+
+bool ShouldStartVerticalTabsCollapsed(PrefService* pref_service) {
+  return pref_service->GetBoolean(prefs::kVerticalTabsStartCollapsed);
+}
+
+const std::string& GetAIWorkspaceGroupPrefix(PrefService* pref_service) {
+  return pref_service->GetString(prefs::kAIWorkspaceGroupPrefix);
+}
+
+bool ShouldShowTabGroupsInBookmarkBar(PrefService* pref_service) {
+  return pref_service->GetBoolean(prefs::kShowTabGroupsInBookmarkBar);
+}
+
+void SyncVerticalTabsPref(PrefService* pref_service) {
+  const bool browseros_enabled =
+      pref_service->GetBoolean(prefs::kVerticalTabsEnabled);
+  const PrefService::Preference* upstream_pref =
+      pref_service->FindPreference(::prefs::kVerticalTabsEnabled);
+  if (upstream_pref && upstream_pref->IsDefaultValue()) {
+    pref_service->SetBoolean(::prefs::kVerticalTabsEnabled, browseros_enabled);
+  }
+}
+
+void ApplyShowTabGroupsInBookmarkBarPref(PrefService* pref_service) {
+  pref_service->SetBoolean(bookmarks::prefs::kShowTabGroupsInBookmarkBar,
+                           ShouldShowTabGroupsInBookmarkBar(pref_service));
+}
+
+void SyncShowTabGroupsInBookmarkBarPref(PrefService* pref_service) {
+  const bool browseros_enabled = ShouldShowTabGroupsInBookmarkBar(pref_service);
+  const PrefService::Preference* upstream_pref = pref_service->FindPreference(
+      bookmarks::prefs::kShowTabGroupsInBookmarkBar);
+  if (upstream_pref && upstream_pref->IsDefaultValue() &&
+      pref_service->GetBoolean(bookmarks::prefs::kShowTabGroupsInBookmarkBar) !=
+          browseros_enabled) {
+    ApplyShowTabGroupsInBookmarkBarPref(pref_service);
+  }
+}
+
+void SyncDefaultTheme(PrefService* pref_service) {
+  const PrefService::Preference* user_color_pref =
+      pref_service->FindPreference(::prefs::kUserColor);
+  if (user_color_pref && user_color_pref->IsDefaultValue()) {
+    pref_service->SetInteger(::prefs::kUserColor,
+                             static_cast<int>(SkColorSetRGB(136, 136, 136)));
+    pref_service->SetString(::prefs::kCurrentThemeID, "user_color_theme_id");
+    pref_service->SetInteger(
+        ::prefs::kBrowserColorVariant,
+        static_cast<int>(ui::mojom::BrowserColorVariant::kNeutral));
+  }
+}
+
+bool IsNtpFocusContentEnabled(PrefService* pref_service) {
+  return pref_service->GetBoolean(prefs::kNtpFocusContent);
+}
+
+bool AutomationNeverStealsFocus(PrefService* pref_service) {
+  return pref_service->GetBoolean(prefs::kAutomationNeverStealsFocus);
+}
+
+bool ShouldPinBrowserOSExtension(const std::string& extension_id,
+                                 PrefService*) {
+  return IsBrowserOSPinnedExtension(extension_id);
+}
+
+}  // namespace browseros
