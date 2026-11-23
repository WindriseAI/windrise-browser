diff --git a/chrome/browser/extensions/extension_service.cc b/chrome/browser/extensions/extension_service.cc
index bbfff3d..feffe76 100644
--- a/chrome/browser/extensions/extension_service.cc
+++ b/chrome/browser/extensions/extension_service.cc
@@ -38,6 +38,7 @@
 #include "build/build_config.h"
 #include "build/chromeos_buildflags.h"
 #include "chrome/browser/browser_process.h"
+#include "chrome/browser/browseros/extensions/browseros_extension_loader.h"
 #include "chrome/browser/extensions/chrome_extension_registrar_delegate.h"
 #include "chrome/browser/extensions/component_loader.h"
 #include "chrome/browser/extensions/corrupted_extension_reinstaller.h"
@@ -374,6 +375,11 @@ void ExtensionService::Init() {
   if (load_command_line_extensions) {
     LoadExtensionsFromCommandLineFlag(switches::kLoadExtension);
+    // Windrise: load bundled signed assistant synchronously when standalone so
+    // the first tab opens Windrise NTP directly; desktop --load-extension wins
+    // above when present.
+    browseros::BrowserOSExtensionLoader::LoadBundledWindriseAssistantIfNeeded(
+        profile_);
   }
   EnabledReloadableExtensions();
   delayed_install_manager_->FinishInstallationsDelayedByShutdown();
   SetReadyAndNotifyListeners();
