diff --git a/chrome/browser/ui/webui/help/version_updater_basic.cc b/chrome/browser/ui/webui/help/version_updater_basic.cc
--- a/chrome/browser/ui/webui/help/version_updater_basic.cc
+++ b/chrome/browser/ui/webui/help/version_updater_basic.cc
@@ -5,10 +5,26 @@
 #include <memory>
 #include <string>
 
+#include "base/i18n/rtl.h"
+#include "base/strings/string_util.h"
+#include "build/build_config.h"
 #include "chrome/browser/ui/webui/help/version_updater.h"
-#include "chrome/browser/upgrade_detector/upgrade_detector.h"
 
 namespace {
 
-// Bare bones implementation just checks if a new version is ready.
+// Windrise Bridge owns browser checks, downloads, verification and installation.
+// DISABLED hides native update controls while still surfacing a calm note on
+// chrome://settings/help explaining that Windrise keeps the browser up to date.
+std::u16string GetWindriseManagedUpdateMessage() {
+  const std::string locale = base::i18n::GetConfiguredLocale();
+  const bool is_chinese =
+      base::EqualsCaseInsensitiveASCII(locale, "zh") ||
+      base::StartsWith(locale, "zh-", base::CompareCase::INSENSITIVE_ASCII) ||
+      base::StartsWith(locale, "zh_", base::CompareCase::INSENSITIVE_ASCII);
+  return is_chinese
+             ? u"由风桥自动保持最新，可在风桥设置中检查更新"
+             : u"Automatically kept up to date by Windrise. You can check for "
+               u"updates in Windrise Settings.";
+}
+
 class VersionUpdaterBasic : public VersionUpdater {
@@ -23,10 +39,18 @@ class VersionUpdaterBasic : public VersionUpdater {
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
+    callback.Run(DISABLED, 0, false, false, std::string(), 0,
+                 GetWindriseManagedUpdateMessage());
   }
+
+#if BUILDFLAG(IS_MAC)
+  void PromoteUpdater() override {}
+#endif
 };
