diff --git a/chrome/browser/extensions/extension_url_overrides_registrar.cc b/chrome/browser/extensions/extension_url_overrides_registrar.cc
index 7762e0c775c22..ede0065069244 100644
--- a/chrome/browser/extensions/extension_url_overrides_registrar.cc
+++ b/chrome/browser/extensions/extension_url_overrides_registrar.cc
@@ -7,6 +7,9 @@
 #include "base/functional/bind.h"
 #include "base/lazy_instance.h"
 #include "base/one_shot_event.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
 #include "chrome/browser/extensions/extension_url_overrides.h"
+#include "chrome/browser/extensions/windrise_extension_protection.h"
 #include "chrome/browser/profiles/profile.h"
+#include "chrome/common/webui_url_constants.h"
 #include "extensions/browser/extension_system.h"
@@ -34,6 +37,30 @@ void ExtensionUrlOverridesRegistrar::OnExtensionLoaded(
     const Extension* extension) {
   const URLOverrides::URLOverrideMap& overrides =
       URLOverrides::GetChromeURLOverrides(extension);
+  // The Windrise assistant takes over the new tab page through
+  // chrome_url_overrides.newtab, but it is loaded from its signed local
+  // directory and never joins the BrowserOS product catalog. Only the
+  // verified assistant may bypass the product gate, and only for the new
+  // tab page: its other chrome:// overrides stay dropped like anyone
+  // else's.
+  const URLOverrides::URLOverrideMap* allowed = &overrides;
+  URLOverrides::URLOverrideMap newtab_only;
+  if (!overrides.empty() &&
+      !browseros::IsActiveBrowserOSExtension(extension->id())) {
+    if (!IsVerifiedWindriseExtension(extension)) {
+      return;
+    }
+    for (const auto& entry : overrides) {
+      if (entry.first == chrome::kChromeUINewTabHost) {
+        newtab_only.insert(entry);
+      }
+    }
+    if (newtab_only.empty()) {
+      return;
+    }
+    allowed = &newtab_only;
+  }
+
   ExtensionUrlOverrides::RegisterOrActivateChromeURLOverrides(
-      Profile::FromBrowserContext(browser_context), overrides);
+      Profile::FromBrowserContext(browser_context), *allowed);
   if (!overrides.empty()) {
