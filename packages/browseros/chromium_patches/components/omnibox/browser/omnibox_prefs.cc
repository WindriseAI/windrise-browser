diff --git a/components/omnibox/browser/omnibox_prefs.cc b/components/omnibox/browser/omnibox_prefs.cc
index 4f601dc..d89653d 100644
--- a/components/omnibox/browser/omnibox_prefs.cc
+++ b/components/omnibox/browser/omnibox_prefs.cc
@@ -23,11 +23,12 @@ void RegisterProfilePrefs(PrefRegistrySimple* registry) {
   registry->RegisterBooleanPref(
       kKeywordSpaceTriggeringEnabled, true,
       user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
+  // Windrise: disable upstream Google Lens and Google AI Mode omnibox buttons.
   registry->RegisterBooleanPref(
-      kShowGoogleLensShortcut, true,
+      kShowGoogleLensShortcut, false,
       user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
   registry->RegisterBooleanPref(
-      kShowAiModeOmniboxButton, true,
+      kShowAiModeOmniboxButton, false,
       user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
   registry->RegisterBooleanPref(
       kShowSearchTools, true, user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
