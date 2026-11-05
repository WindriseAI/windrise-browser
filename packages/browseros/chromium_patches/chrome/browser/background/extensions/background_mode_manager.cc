diff --git a/chrome/browser/background/extensions/background_mode_manager.cc b/chrome/browser/background/extensions/background_mode_manager.cc
index 16e28b4..b68bca4 100644
--- a/chrome/browser/background/extensions/background_mode_manager.cc
+++ b/chrome/browser/background/extensions/background_mode_manager.cc
@@ -351,7 +351,7 @@ BackgroundModeManager::~BackgroundModeManager() {
 
 // static
 void BackgroundModeManager::RegisterPrefs(PrefRegistrySimple* registry) {
-  registry->RegisterBooleanPref(prefs::kBackgroundModeEnabled, true);
+  registry->RegisterBooleanPref(prefs::kBackgroundModeEnabled, false);
 }
 
 void BackgroundModeManager::RegisterProfile(Profile* profile) {
