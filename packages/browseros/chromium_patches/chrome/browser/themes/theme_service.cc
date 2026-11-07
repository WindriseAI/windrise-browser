diff --git a/chrome/browser/themes/theme_service.cc b/chrome/browser/themes/theme_service.cc
index cc3523c..ea0a3d9 100644
--- a/chrome/browser/themes/theme_service.cc
+++ b/chrome/browser/themes/theme_service.cc
@@ -34,6 +34,7 @@
 #include "base/trace_event/trace_event.h"
 #include "build/build_config.h"
 #include "chrome/browser/browser_process.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
 #include "chrome/browser/infobars/browser_infobar_manager.h"
 #include "chrome/browser/infobars/infobar_features.h"
 #include "chrome/browser/infobars/infobar_spec.h"
@@ -321,7 +322,7 @@ void ThemeService::RegisterProfilePrefs(
   registry->RegisterIntegerPref(themes::kPolicyThemeColor, SK_ColorTRANSPARENT);
   registry->RegisterIntegerPref(
       prefs::kBrowserColorScheme,
-      std::to_underlying(ThemeService::BrowserColorScheme::kSystem));
+      std::to_underlying(ThemeService::BrowserColorScheme::kLight));
   registry->RegisterIntegerPref(prefs::kUserColor, SK_ColorTRANSPARENT);
   registry->RegisterIntegerPref(
       prefs::kBrowserColorVariant,
@@ -346,6 +347,9 @@ ThemeService::~ThemeService() = default;
 void ThemeService::Init() {
   theme_helper_->DCheckCalledOnValidSequence();
 
+  // Seed the BrowserOS default before ThemeService builds its initial theme.
+  // SyncDefaultTheme only seeds the color while its preference is default.
+  browseros::SyncDefaultTheme(profile_->GetPrefs());
   InitFromPrefs();
 
   // ThemeObserver should be constructed before calling
