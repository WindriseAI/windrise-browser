diff --git a/chrome/browser/background/extensions/background_mode_manager.cc b/chrome/browser/background/extensions/background_mode_manager.cc
--- a/chrome/browser/background/extensions/background_mode_manager.cc
+++ b/chrome/browser/background/extensions/background_mode_manager.cc
@@ -348,9 +348,9 @@ void BackgroundModeManager::RegisterPrefs(PrefRegistrySimple* registry) {
 
 // static
 void BackgroundModeManager::RegisterPrefs(PrefRegistrySimple* registry) {
-  registry->RegisterBooleanPref(prefs::kBackgroundModeEnabled, true);
+  registry->RegisterBooleanPref(prefs::kBackgroundModeEnabled, false);
 }
 
 void BackgroundModeManager::RegisterProfile(Profile* profile) {
   // We don't want to register multiple times for one profile.
   DCHECK(!background_mode_data_.contains(profile));
