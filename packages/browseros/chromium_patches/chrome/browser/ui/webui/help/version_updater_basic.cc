diff --git a/chrome/browser/ui/webui/help/version_updater_basic.cc b/chrome/browser/ui/webui/help/version_updater_basic.cc
--- a/chrome/browser/ui/webui/help/version_updater_basic.cc
+++ b/chrome/browser/ui/webui/help/version_updater_basic.cc
@@ -1,35 +1,80 @@
-// Copyright 2012 The Chromium Authors
-// Use of this source code is governed by a BSD-style license that can be
-// found in the LICENSE file.
-
-#include <memory>
-#include <string>
-
-#include "chrome/browser/ui/webui/help/version_updater.h"
-#include "chrome/browser/upgrade_detector/upgrade_detector.h"
-
-namespace {
-
-// Bare bones implementation just checks if a new version is ready.
-class VersionUpdaterBasic : public VersionUpdater {
- public:
-  VersionUpdaterBasic(const VersionUpdaterBasic&) = delete;
-  VersionUpdaterBasic& operator=(const VersionUpdaterBasic&) = delete;
-  VersionUpdaterBasic() = default;
-  ~VersionUpdaterBasic() override = default;
-
-  // VersionUpdater implementation.
-  void CheckForUpdate(StatusCallback callback, PromoteCallback) override {
-    const Status status = UpgradeDetector::GetInstance()->is_upgrade_available()
-                              ? NEARLY_UPDATED
-                              : DISABLED;
-    callback.Run(status, 0, false, false, std::string(), 0, std::u16string());
-  }
-};
-
-}  // namespace
-
-std::unique_ptr<VersionUpdater> VersionUpdater::Create(
-    content::WebContents* web_contents) {
-  return std::make_unique<VersionUpdaterBasic>();
-}
+// Copyright 2012 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include <memory>
+#include <string>
+
+#include "base/memory/weak_ptr.h"
+#include "build/build_config.h"
+#include "chrome/browser/extensions/windrise_extension_protection.h"
+#include "chrome/browser/ui/webui/help/version_updater.h"
+#include "content/public/browser/page_navigator.h"
+#include "content/public/browser/web_contents.h"
+#include "extensions/browser/extension_registry.h"
+#include "extensions/common/extension.h"
+
+namespace {
+
+constexpr char kAssistantId[] = "amlnokjigkbgngdgkofikfphjbknfodp";
+
+// The signed component updater in Windrise Bridge owns checks, downloads and
+// installation. Native About only hands the user to its existing settings page;
+// opening that page is never evidence of an up-to-date browser.
+class VersionUpdaterWindrise : public VersionUpdater {
+ public:
+  explicit VersionUpdaterWindrise(content::WebContents* web_contents)
+      : web_contents_(web_contents ? web_contents->GetWeakPtr() : nullptr) {}
+
+  void CheckForUpdate(StatusCallback callback,
+                      PromoteCallback promote_callback) override {
+#if BUILDFLAG(IS_MAC)
+    if (!promote_callback.is_null()) {
+      promote_callback.Run(PROMOTE_HIDDEN);
+    }
+#endif
+    if (!handed_over_) {
+      handed_over_ = OpenManagedUpdatePage();
+    }
+    callback.Run(handed_over_ ? DISABLED : FAILED, 0, false, false,
+                 std::string(), 0, std::u16string());
+  }
+
+#if BUILDFLAG(IS_MAC)
+  // Never provision a second system updater, even if an old UI calls promote.
+  void PromoteUpdater() override { OpenManagedUpdatePage(); }
+#endif
+
+ private:
+  bool OpenManagedUpdatePage() {
+    if (!web_contents_) {
+      return false;
+    }
+    auto* registry = extensions::ExtensionRegistry::Get(
+        web_contents_->GetBrowserContext());
+    const auto* assistant = registry
+        ? registry->enabled_extensions().GetByID(kAssistantId)
+        : nullptr;
+    if (!extensions::IsAuthorizedWindriseExtension(assistant)) {
+      return false;
+    }
+    // This existing page exposes browser-package:refresh and install through
+    // the protected worker binding and the host's existing RPC allowlist.
+    const GURL url = assistant->GetResourceURL("settings.html").Resolve("#general");
+    const content::OpenURLParams params =
+        content::OpenURLParams::CreateBrowserInitiated(
+            url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
+            ui::PAGE_TRANSITION_LINK);
+    return web_contents_->OpenURL(params, {}) != nullptr;
+  }
+
+  base::WeakPtr<content::WebContents> web_contents_;
+  bool handed_over_ = false;
+};
+
+}  // namespace
+
+std::unique_ptr<VersionUpdater> VersionUpdater::Create(
+    content::WebContents* web_contents) {
+  return std::make_unique<VersionUpdaterWindrise>(web_contents);
+}
