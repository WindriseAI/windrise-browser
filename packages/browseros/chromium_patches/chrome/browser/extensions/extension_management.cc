diff --git a/chrome/browser/extensions/extension_management.cc b/chrome/browser/extensions/extension_management.cc
index 1245788078fc00e9a2069e0b6da1838aba573961..f53fe344066c7fc3280190d07d04c8c28a12c05d 100644
--- a/chrome/browser/extensions/extension_management.cc
+++ b/chrome/browser/extensions/extension_management.cc
@@ -25,6 +25,7 @@
 #include "base/values.h"
 #include "base/version.h"
 #include "build/chromeos_buildflags.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
 #include "chrome/browser/enterprise/util/managed_browser_utils.h"
 #include "chrome/browser/extensions/cws_info_service_factory.h"
 #include "chrome/browser/extensions/extension_management_constants.h"
@@ -329,6 +330,12 @@ bool ExtensionManagement::IsUpdateUrlOverridden(const ExtensionId& id) {
 }
 
 GURL ExtensionManagement::GetEffectiveUpdateURL(const Extension& extension) {
+  // Bundled extensions update with the application package. The updater also
+  // excludes these IDs because an empty URL alone falls back to the web store.
+  if (browseros::IsBrowserOSExtensionUpdateDisabled(extension.id())) {
+    return GURL();
+  }
+
   if (IsUpdateUrlOverridden(extension.id())) {
     DCHECK(!extension.was_installed_by_default())
         << "Update URL should not be overridden for default-installed "
@@ -692,6 +699,14 @@ ExtensionIdSet ExtensionManagement::GetForcePinnedList() const {
       force_pinned_list.insert(entry.first);
     }
   }
+
+  // Always force-pin BrowserOS extensions that are marked pinned.
+  for (const auto& extension_id : browseros::GetActiveBrowserOSExtensionIds()) {
+    if (browseros::IsBrowserOSPinnedExtension(extension_id)) {
+      force_pinned_list.insert(extension_id);
+    }
+  }
+
   return force_pinned_list;
 }
 
