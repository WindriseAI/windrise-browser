diff --git a/chrome/browser/ui/webui/help/version_updater_basic.cc b/chrome/browser/ui/webui/help/version_updater_basic.cc
--- a/chrome/browser/ui/webui/help/version_updater_basic.cc
+++ b/chrome/browser/ui/webui/help/version_updater_basic.cc
@@ -5,10 +5,11 @@
 #include <memory>
 #include <string>
 
+#include "build/build_config.h"
 #include "chrome/browser/ui/webui/help/version_updater.h"
-#include "chrome/browser/upgrade_detector/upgrade_detector.h"
 
 namespace {
 
-// Bare bones implementation just checks if a new version is ready.
+// Windrise Bridge owns browser checks, downloads, verification and installation.
+// DISABLED hides the native About update controls without claiming it is current.
 class VersionUpdaterBasic : public VersionUpdater {
@@ -23,10 +24,17 @@ class VersionUpdaterBasic : public VersionUpdater {
   ~VersionUpdaterBasic() override = default;
 
   // VersionUpdater implementation.
-  void CheckForUpdate(StatusCallback callback, PromoteCallback) override {
-    const Status status = UpgradeDetector::GetInstance()->is_upgrade_available()
-                              ? NEARLY_UPDATED
-                              : DISABLED;
-    callback.Run(status, 0, false, false, std::string(), 0, std::u16string());
+  void CheckForUpdate(StatusCallback callback,
+                      PromoteCallback promote_callback) override {
+#if BUILDFLAG(IS_MAC)
+    if (!promote_callback.is_null()) {
+      promote_callback.Run(PROMOTE_HIDDEN);
+    }
+#endif
+    callback.Run(DISABLED, 0, false, false, std::string(), 0, std::u16string());
   }
+
+#if BUILDFLAG(IS_MAC)
+  void PromoteUpdater() override {}
+#endif
 };
