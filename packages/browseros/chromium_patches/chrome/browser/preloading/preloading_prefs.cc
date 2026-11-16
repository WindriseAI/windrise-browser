diff --git a/chrome/browser/preloading/preloading_prefs.cc b/chrome/browser/preloading/preloading_prefs.cc
index 69ba78f694..12fbe21a8f 100644
--- a/chrome/browser/preloading/preloading_prefs.cc
+++ b/chrome/browser/preloading/preloading_prefs.cc
@@ -16,7 +16,7 @@ void RegisterPredictionOptionsProfilePrefs(
     user_prefs::PrefRegistrySyncable* registry) {
   registry->RegisterIntegerPref(
       prefs::kNetworkPredictionOptions,
-      static_cast<int>(NetworkPredictionOptions::kDefault),
+      static_cast<int>(NetworkPredictionOptions::kDisabled),
       user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
 }
 
