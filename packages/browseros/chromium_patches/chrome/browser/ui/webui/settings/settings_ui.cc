diff --git a/chrome/browser/ui/webui/settings/settings_ui.cc b/chrome/browser/ui/webui/settings/settings_ui.cc
index 1437a64..a38d8ec 100644
--- a/chrome/browser/ui/webui/settings/settings_ui.cc
+++ b/chrome/browser/ui/webui/settings/settings_ui.cc
@@ -66,6 +66,7 @@
 #include "chrome/browser/ui/webui/settings/accessibility_main_handler.h"
 #include "chrome/browser/ui/webui/settings/appearance_handler.h"
 #include "chrome/browser/ui/webui/settings/browser_lifetime_handler.h"
+#include "chrome/browser/ui/webui/settings/browseros_metrics_handler.h"
 #include "chrome/browser/ui/webui/settings/device_tab_visibility_handler.h"
 #include "chrome/browser/ui/webui/settings/dictation_handler.h"
 #include "chrome/browser/ui/webui/settings/downloads_handler.h"
@@ -257,6 +258,8 @@ void SettingsUI::RegisterProfilePrefs(
   registry->RegisterBooleanPref(prefs::kImportDialogSavedPasswords, true);
   registry->RegisterBooleanPref(prefs::kImportDialogSearchEngine, true);
   registry->RegisterDictionaryPref(prefs::kDeviceTabVisibilityHiddenSessions);
+  registry->RegisterBooleanPref(prefs::kImportDialogExtensions, true);
+  registry->RegisterBooleanPref(prefs::kImportDialogCookies, true);
 }
 
 SettingsUI::SettingsUI(content::WebUI* web_ui)
@@ -328,6 +331,7 @@ SettingsUI::SettingsUI(content::WebUI* web_ui)
 #if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
   AddSettingsPageUIHandler(std::make_unique<PasskeysHandler>());
 #endif
+  AddSettingsPageUIHandler(std::make_unique<BrowserOSMetricsHandler>());
 
 #if BUILDFLAG(IS_CHROMEOS)
   InitBrowserSettingsWebUIHandlers();
