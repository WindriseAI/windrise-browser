diff --git a/chrome/browser/spellchecker/spellcheck_factory.cc b/chrome/browser/spellchecker/spellcheck_factory.cc
index 0000000000..1111111111 100644
--- a/chrome/browser/spellchecker/spellcheck_factory.cc
+++ b/chrome/browser/spellchecker/spellcheck_factory.cc
@@ -72,7 +72,8 @@ void SpellcheckServiceFactory::RegisterProfilePrefs(
 #else
   uint32_t flags = user_prefs::PrefRegistrySyncable::SYNCABLE_PREF;
 #endif
-  user_prefs->RegisterBooleanPref(spellcheck::prefs::kSpellCheckEnable, true,
+  // Windrise (R728): disable spellcheck dictionary downloads by default.
+  user_prefs->RegisterBooleanPref(spellcheck::prefs::kSpellCheckEnable, false,
                                   flags);
 }
 
