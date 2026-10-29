diff --git a/chrome/browser/ui/webui/settings/settings_ui.cc b/chrome/browser/ui/webui/settings/settings_ui.cc
index 781744f9cfdb691b41299d8b07c984888605366a..6ef237163c80ff769a5f7e91ff4e4ae2bac828bc 100644
--- a/chrome/browser/ui/webui/settings/settings_ui.cc
+++ b/chrome/browser/ui/webui/settings/settings_ui.cc
@@ -65,6 +65,7 @@
 #include "chrome/browser/ui/webui/settings/accessibility_main_handler.h"
 #include "chrome/browser/ui/webui/settings/appearance_handler.h"
 #include "chrome/browser/ui/webui/settings/browser_lifetime_handler.h"
+#include "chrome/browser/ui/webui/settings/browseros_metrics_handler.h"
 #include "chrome/browser/ui/webui/settings/dictation_handler.h"
 #include "chrome/browser/ui/webui/settings/downloads_handler.h"
 #include "chrome/browser/ui/webui/settings/font_handler.h"
@@ -231,6 +232,8 @@ void SettingsUI::RegisterProfilePrefs(
   registry->RegisterBooleanPref(prefs::kImportDialogHistory, true);
   registry->RegisterBooleanPref(prefs::kImportDialogSavedPasswords, true);
   registry->RegisterBooleanPref(prefs::kImportDialogSearchEngine, true);
+  registry->RegisterBooleanPref(prefs::kImportDialogExtensions, true);
+  registry->RegisterBooleanPref(prefs::kImportDialogCookies, true);
 }
 
 SettingsUI::SettingsUI(content::WebUI* web_ui)
@@ -297,6 +300,7 @@ SettingsUI::SettingsUI(content::WebUI* web_ui)
 #if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
   AddSettingsPageUIHandler(std::make_unique<PasskeysHandler>());
 #endif
+  AddSettingsPageUIHandler(std::make_unique<BrowserOSMetricsHandler>());
 
 #if BUILDFLAG(IS_CHROMEOS)
   InitBrowserSettingsWebUIHandlers();
