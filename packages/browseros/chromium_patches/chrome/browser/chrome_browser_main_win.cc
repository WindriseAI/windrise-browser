diff --git a/chrome/browser/chrome_browser_main_win.cc b/chrome/browser/chrome_browser_main_win.cc
index 7784475..7ba445d 100644
--- a/chrome/browser/chrome_browser_main_win.cc
+++ b/chrome/browser/chrome_browser_main_win.cc
@@ -58,6 +58,7 @@
 #include "chrome/browser/active_use_util.h"
 #include "chrome/browser/browser_features.h"
 #include "chrome/browser/browser_process.h"
+#include "chrome/browser/buildflags.h"
 #include "chrome/browser/device_reauth/win/authenticator_win.h"
 #include "chrome/browser/device_reauth/win/device_auth_availability_checker_win.h"
 #include "chrome/browser/first_run/first_run.h"
@@ -140,6 +141,10 @@
 #include "chrome/browser/platform_experience/installer/installer_win.h"
 #endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)
 
+#if BUILDFLAG(ENABLE_WINSPARKLE)
+#include "chrome/browser/win/winsparkle_glue.h"
+#endif  // BUILDFLAG(ENABLE_WINSPARKLE)
+
 namespace {
 
 constexpr base::TimeDelta kCacheBiometricsAvailabilityDelay = base::Seconds(30);
@@ -738,6 +743,11 @@ int ChromeBrowserMainPartsWin::PostCreateThreads() {
 }
 
 void ChromeBrowserMainPartsWin::PostMainMessageLoopRun() {
+#if BUILDFLAG(ENABLE_WINSPARKLE)
+  // Shut WinSparkle down while the task system is still alive.
+  winsparkle_glue::Cleanup();
+#endif
+
   base::ImportantFileWriterCleaner::GetInstance().Stop();
 
   ChromeBrowserMainParts::PostMainMessageLoopRun();
@@ -798,6 +808,12 @@ void ChromeBrowserMainPartsWin::PostBrowserStart() {
 
   InitializeChromeElf();
 
+#if BUILDFLAG(ENABLE_WINSPARKLE)
+  // Start the WinSparkle auto-updater. Must come after browser start so its
+  // first automatic check (and any update UI) runs behind a visible browser.
+  winsparkle_glue::Initialize();
+#endif
+
 #if BUILDFLAG(USE_GOOGLE_UPDATE_INTEGRATION)
   if constexpr (kShouldRecordActiveUse) {
     did_run_updater_.emplace();
