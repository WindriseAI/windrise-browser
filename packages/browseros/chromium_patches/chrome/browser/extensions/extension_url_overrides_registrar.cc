diff --git a/chrome/browser/extensions/extension_url_overrides_registrar.cc b/chrome/browser/extensions/extension_url_overrides_registrar.cc
index 7762e0c775c22..ede0065069244 100644
--- a/chrome/browser/extensions/extension_url_overrides_registrar.cc
+++ b/chrome/browser/extensions/extension_url_overrides_registrar.cc
@@ -5,15 +5,81 @@
 #include "chrome/browser/extensions/extension_url_overrides_registrar.h"

+#include <string_view>
+#include <vector>
+
 #include "base/functional/bind.h"
 #include "base/lazy_instance.h"
+#include "base/memory/weak_ptr.h"
 #include "base/one_shot_event.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
+#include "chrome/browser/extensions/extension_tab_util.h"
 #include "chrome/browser/extensions/extension_url_overrides.h"
+#include "chrome/browser/extensions/windrise_extension_protection.h"
 #include "chrome/browser/profiles/profile.h"
+#include "chrome/common/webui_url_constants.h"
+#include "content/public/browser/navigation_controller.h"
+#include "content/public/browser/navigation_entry.h"
+#include "content/public/browser/web_contents.h"
+#include "content/public/common/url_constants.h"
 #include "extensions/browser/extension_system.h"
 #include "extensions/buildflags/buildflags.h"
+#include "ui/base/page_transition_types.h"
+#include "url/gurl.h"

 static_assert(BUILDFLAG(ENABLE_EXTENSIONS_CORE));

 namespace extensions {

+namespace {
+
+bool IsFallbackNewTabUrl(const GURL& url) {
+  if (!url.is_valid() || !url.SchemeIs(content::kChromeUIScheme)) {
+    return false;
+  }
+  const std::string_view host = url.host();
+  return host == chrome::kChromeUINewTabHost ||
+         host == chrome::kChromeUINewTabPageHost ||
+         host == chrome::kChromeUINewTabPageThirdPartyHost;
+}
+
+void CollectFallbackNewTab(
+    Profile* target_profile,
+    std::vector<base::WeakPtr<content::WebContents>>* out,
+    content::WebContents* web_contents) {
+  if (!web_contents || web_contents->GetBrowserContext() != target_profile) {
+    return;
+  }
+  content::NavigationController& controller = web_contents->GetController();
+  const content::NavigationEntry* entry = controller.GetPendingEntry();
+  if (!entry) {
+    entry = controller.GetLastCommittedEntry();
+  }
+  if (entry && IsFallbackNewTabUrl(entry->GetURL())) {
+    out->push_back(web_contents->GetWeakPtr());
+  }
+}
+
+void UpgradeFallbackNewTabPages(Profile* profile) {
+  if (!profile || profile->IsOffTheRecord()) {
+    return;
+  }
+  std::vector<base::WeakPtr<content::WebContents>> fallback_tabs;
+  ExtensionTabUtil::ForEachTab(base::BindRepeating(
+      &CollectFallbackNewTab, base::Unretained(profile),
+      base::Unretained(&fallback_tabs)));
+  const GURL newtab_url(chrome::kChromeUINewTabURL);
+  for (const base::WeakPtr<content::WebContents>& weak_contents :
+       fallback_tabs) {
+    if (!weak_contents) {
+      continue;
+    }
+    content::NavigationController::LoadURLParams params(newtab_url);
+    params.transition_type = ui::PAGE_TRANSITION_AUTO_TOPLEVEL;
+    params.should_replace_current_entry = true;
+    weak_contents->GetController().LoadURLWithParams(params);
+  }
+}
+
+}  // namespace
+
 ExtensionUrlOverridesRegistrar::ExtensionUrlOverridesRegistrar(
@@ -34,6 +100,33 @@ void ExtensionUrlOverridesRegistrar::OnExtensionLoaded(
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
+  if (!newtab_only.empty()) {
+    UpgradeFallbackNewTabPages(Profile::FromBrowserContext(browser_context));
+  }
   if (!overrides.empty()) {
