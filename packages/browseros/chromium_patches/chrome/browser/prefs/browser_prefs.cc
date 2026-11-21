diff --git a/chrome/browser/prefs/browser_prefs.cc b/chrome/browser/prefs/browser_prefs.cc
index 2653e37becbfa113d92456139d86c321f09ed8b5..7f9327317554c866ac283630291d5cfb3b824bb9 100644
--- a/chrome/browser/prefs/browser_prefs.cc
+++ b/chrome/browser/prefs/browser_prefs.cc
@@ -24,7 +24,13 @@
 #include "chrome/browser/accessibility/page_colors_controller.h"
 #include "chrome/browser/accessibility/prefers_default_scrollbar_styles_prefs.h"
+#include "base/values.h"
 #include "chrome/browser/browser_process_impl.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
+#include "chrome/browser/browseros/customization/customization_prefs.h"
+#include "chrome/browser/browseros/server/browseros_server_prefs.h"
 #include "chrome/browser/chrome_content_browser_client.h"
 #include "chrome/browser/component_updater/component_updater_prefs.h"
 #include "chrome/browser/context_hub/prefs.h"
+#include "components/network_time/network_time_pref_names.h"
+#include "components/translate/core/browser/translate_pref_names.h"
 #include "chrome/browser/contextual_cueing/prefs.h"
@@ -1424,6 +1430,7 @@ void RegisterLocalState(PrefRegistrySimple* registry) {
   breadcrumbs::RegisterPrefs(registry);
   browser_shutdown::RegisterPrefs(registry);
   BrowserProcessImpl::RegisterPrefs(registry);
+  browseros_server::RegisterLocalStatePrefs(registry);
   ChromeContentBrowserClient::RegisterLocalStatePrefs(registry);
 #if BUILDFLAG(CHROME_FOR_TESTING)
   chrome_for_testing::RegisterPrefs(registry);
@@ -1754,7 +1761,10 @@ void RegisterLocalState(PrefRegistrySimple* registry) {
   memory::EnterpriseMemoryLimitPrefObserver::RegisterPrefs(registry);
   metrics::RegisterDemographicsLocalStatePrefs(registry);
   metrics::TabStatsTracker::RegisterPrefs(registry);
   network_time::NetworkTimeTracker::RegisterPrefs(registry);
+  // Windrise (R728): disable background Google network time queries by default.
+  registry->SetDefaultPrefValue(network_time::prefs::kNetworkTimeQueriesEnabled,
+                                base::Value(false));
   omnibox::RegisterLocalStatePrefs(registry);
 #if !BUILDFLAG(IS_ANDROID)
   omnibox_everywhere::prefs::RegisterLocalStatePrefs(registry);
@@ -1852,6 +1862,7 @@ void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
 #if !BUILDFLAG(IS_ANDROID)
   indigo::prefs::RegisterProfilePrefs(registry);
 #endif
+  RegisterBrowserOSPrefs(registry);
   RegisterPrefersDefaultScrollbarStylesPrefs(registry);
   RegisterSafetyHubProfilePrefs(registry);
 #if BUILDFLAG(IS_CHROMEOS)
@@ -2170,7 +2181,9 @@ void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
 #if BUILDFLAG(SAFE_BROWSING_AVAILABLE)
   safe_browsing::file_type::RegisterProfilePrefs(registry);
 #endif
   safe_browsing::RegisterProfilePrefs(registry);
+  // Windrise (R728): disable Safe Browsing by default (user can re-enable in Settings).
+  registry->SetDefaultPrefValue(prefs::kSafeBrowsingEnabled, base::Value(false));
   safety_check::prefs::RegisterProfilePrefs(registry);
   SearchPrefetchService::RegisterProfilePrefs(registry);
   blocked_content::SafeBrowsingTriggeredPopupBlocker::RegisterProfilePrefs(
@@ -2200,6 +2213,9 @@ void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
   TemplateURLPrepopulateData::RegisterProfilePrefs(registry);
   tab_groups::prefs::RegisterProfilePrefs(registry);
   translate::TranslatePrefs::RegisterProfilePrefs(registry);
+  // Windrise (R728): disable Google Translate by default (user can re-enable in Settings).
+  registry->SetDefaultPrefValue(translate::prefs::kOfferTranslateEnabled,
+                                base::Value(false));
   ttc::RegisterProfilePrefs(registry);
   universal_optout::prefs::RegisterProfilePrefs(registry);
   visited_url_ranking::GroupSuggestionsServiceImpl::RegisterProfilePrefs(
@@ -2307,6 +2323,16 @@ void RegisterGeminiSettingsPrefs(user_prefs::PrefRegistrySyncable* registry) {
   registry->RegisterIntegerPref(optimization_guide::prefs::kGeminiSettings, 0);
 }
 
+void RegisterBrowserOSPrefs(user_prefs::PrefRegistrySyncable* registry) {
+  browseros::RegisterProfilePrefs(registry);
+  browseros::RegisterCustomizationProfilePrefs(registry);
+}
+
+void MigrateBrowserOSProfilePrefs(PrefService* profile_prefs) {
+  browseros::MigrateProfilePrefs(profile_prefs);
+  browseros::MigrateCustomizationProfilePrefs(profile_prefs);
+}
+
 #if BUILDFLAG(IS_CHROMEOS)
 void RegisterSigninProfilePrefs(user_prefs::PrefRegistrySyncable* registry,
                                 std::string_view country) {
