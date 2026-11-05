diff --git a/chrome/browser/extensions/external_provider_manager.cc b/chrome/browser/extensions/external_provider_manager.cc
index ca6adb4..e30b9e4 100644
--- a/chrome/browser/extensions/external_provider_manager.cc
+++ b/chrome/browser/extensions/external_provider_manager.cc
@@ -4,6 +4,7 @@
 
 #include "chrome/browser/extensions/external_provider_manager.h"
 
+#include <algorithm>
 #include <cstddef>
 
 #include "base/check.h"
@@ -15,6 +16,7 @@
 #include "base/trace_event/trace_event.h"
 #include "base/version.h"
 #include "build/build_config.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
 #include "chrome/browser/extensions/corrupted_extension_reinstaller.h"
 #include "chrome/browser/extensions/extension_error_controller.h"
 #include "chrome/browser/extensions/extension_management.h"
@@ -302,6 +304,7 @@ bool ExternalProviderManager::OnExternalExtensionFileFound(
   installer->set_expected_version(info.version,
                                   true /* fail_install_if_unexpected */);
   installer->set_install_immediately(info.install_immediately);
+  installer->set_external_install_priority(info.install_priority);
   installer->set_creation_flags(info.creation_flags);
 
   CRXFileInfo file_info(
@@ -496,8 +499,21 @@ void ExternalProviderManager::OnExternalProviderUpdateComplete(
   Profile* profile = Profile::FromBrowserContext(context_);
   ExtensionUpdater* updater = ExtensionUpdater::Get(profile);
   if (!update_url_extensions.empty() && updater->enabled()) {
-    // Empty params will cause pending extensions to be updated.
-    updater->CheckNow(ExtensionUpdater::CheckParams());
+    ExtensionUpdater::CheckParams params;
+    if (std::ranges::all_of(update_url_extensions, [](const auto& extension) {
+          return browseros::IsActiveBrowserOSExtension(extension.extension_id);
+        })) {
+      // Product pages such as the cockpit can stay open indefinitely, so an
+      // idle-only update can remain downloaded without ever activating. Reuse
+      // this provider's check and force activation only for its active product
+      // IDs; empty/default params would include unrelated installed extensions.
+      for (const auto& extension : update_url_extensions) {
+        params.ids.push_back(extension.extension_id);
+      }
+      params.install_immediately = true;
+      params.fetch_priority = DownloadFetchPriority::kForeground;
+    }
+    updater->CheckNow(std::move(params));
   }
 
   error_controller_->ShowErrorIfNeeded();
