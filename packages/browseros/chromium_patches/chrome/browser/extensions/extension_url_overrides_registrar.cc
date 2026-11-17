diff --git a/chrome/browser/extensions/extension_url_overrides_registrar.cc b/chrome/browser/extensions/extension_url_overrides_registrar.cc
index 7762e0c775c22..ede0065069244 100644
--- a/chrome/browser/extensions/extension_url_overrides_registrar.cc
+++ b/chrome/browser/extensions/extension_url_overrides_registrar.cc
@@ -7,6 +7,8 @@
 #include "base/functional/bind.h"
 #include "base/lazy_instance.h"
 #include "base/one_shot_event.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
 #include "chrome/browser/extensions/extension_url_overrides.h"
+#include "chrome/browser/extensions/windrise_extension_protection.h"
 #include "chrome/browser/profiles/profile.h"
 #include "extensions/browser/extension_system.h"
@@ -34,6 +36,17 @@ void ExtensionUrlOverridesRegistrar::OnExtensionLoaded(
     const Extension* extension) {
   const URLOverrides::URLOverrideMap& overrides =
       URLOverrides::GetChromeURLOverrides(extension);
+  // The Windrise assistant takes over the new tab page through
+  // chrome_url_overrides.newtab, but it is loaded from its signed local
+  // directory and never joins the BrowserOS product catalog. Only the
+  // verified assistant may bypass the product gate here; every other
+  // extension's overrides stay dropped.
+  if (!overrides.empty() &&
+      !browseros::IsActiveBrowserOSExtension(extension->id()) &&
+      !IsVerifiedWindriseExtension(extension)) {
+    return;
+  }
+
   ExtensionUrlOverrides::RegisterOrActivateChromeURLOverrides(
       Profile::FromBrowserContext(browser_context), overrides);
   if (!overrides.empty()) {
