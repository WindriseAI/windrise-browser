diff --git a/chrome/browser/chrome_content_browser_client.cc b/chrome/browser/chrome_content_browser_client.cc
index 565bd58..a847ff7 100644
--- a/chrome/browser/chrome_content_browser_client.cc
+++ b/chrome/browser/chrome_content_browser_client.cc
@@ -676,6 +676,7 @@
 #endif
 
 #if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
+#include "chrome/browser/browseros/core/browseros_constants.h"
 #include "chrome/browser/extensions/chrome_content_browser_client_extensions_part.h"
 #include "chrome/browser/extensions/chrome_extension_cookies.h"
 #include "chrome/browser/extensions/extension_util.h"
@@ -1641,7 +1642,7 @@ void ChromeContentBrowserClient::RegisterLocalStatePrefs(
 void ChromeContentBrowserClient::RegisterProfilePrefs(
     user_prefs::PrefRegistrySyncable* registry) {
   registry->RegisterBooleanPref(prefs::kDisable3DAPIs, false);
-  registry->RegisterBooleanPref(prefs::kEnableHyperlinkAuditing, true);
+  registry->RegisterBooleanPref(prefs::kEnableHyperlinkAuditing, false);
   // Register user prefs for mapping SitePerProcess and IsolateOrigins in
   // user policy in addition to the same named ones in Local State (which are
   // used for mapping the command-line flags).
@@ -5225,6 +5226,52 @@ ChromeContentBrowserClient::GetForcedColorsForWebContents(
       .forced_colors;
 }
 
+// Handles chrome://windrise/* URLs by rewriting to extension URLs.
+// Forward handler: chrome://windrise/settings#grids ->
+// chrome-extension://[id]/settings.html#grids
+static bool HandleBrowserOSURL(GURL* url,
+                               content::BrowserContext* browser_context) {
+  if (!url->SchemeIs(content::kChromeUIScheme) ||
+      url->host() != browseros::kBrowserOSHost) {
+    return false;
+  }
+
+  std::string extension_url = browseros::GetBrowserOSExtensionURL(url->path());
+  if (extension_url.empty()) {
+    return false;
+  }
+
+  // Preserve virtual URL fragment (e.g. chrome://windrise/settings#grids) when
+  // the route itself does not pin a hash.
+  if (url->has_ref() && !url->ref().empty() &&
+      extension_url.find('#') == std::string::npos) {
+    extension_url.push_back('#');
+    extension_url.append(url->ref());
+  }
+
+  *url = GURL(extension_url);
+  return true;
+}
+
+// Reverse handler: chrome-extension://[id]/options.html#ai ->
+// chrome://browseros/ai This ensures the virtual URL is shown in the address
+// bar.
+static bool ReverseBrowserOSURL(GURL* url,
+                                content::BrowserContext* browser_context) {
+  if (!url->SchemeIs(extensions::kExtensionScheme)) {
+    return false;
+  }
+
+  std::string virtual_url =
+      browseros::GetBrowserOSVirtualURL(url->host(), url->path(), url->ref());
+  if (virtual_url.empty()) {
+    return false;
+  }
+
+  *url = GURL(virtual_url);
+  return true;
+}
+
 void ChromeContentBrowserClient::BrowserURLHandlerCreated(
     BrowserURLHandler* handler) {
   // The group policy NTP URL handler must be registered before the other NTP
@@ -5241,6 +5288,13 @@ void ChromeContentBrowserClient::BrowserURLHandlerCreated(
   handler->AddHandlerPair(&HandleChromeAboutAndChromeSyncRewrite,
                           BrowserURLHandler::null_handler());
 
+  // Handler to rewrite chrome://windrise/* to extension URLs.
+  handler->AddHandlerPair(&HandleBrowserOSURL, &ReverseBrowserOSURL);
+  // Reverse-only handler for when extension opens its URL directly
+  // (e.g., chrome.tabs.create({url: 'settings.html#grids'}))
+  handler->AddHandlerPair(BrowserURLHandler::null_handler(),
+                          &ReverseBrowserOSURL);
+
 #if BUILDFLAG(IS_ANDROID)
   // Handler to rewrite chrome://newtab on Android.
   handler->AddHandlerPair(&chrome::android::HandleAndroidNativePageURL,
@@ -8453,6 +8507,15 @@ content::ContentBrowserClient::LocalNetworkAccessRequestPolicyOverride
 ChromeContentBrowserClient::ShouldOverrideLocalNetworkAccessRequestPolicy(
     content::BrowserContext* browser_context,
     const url::Origin& origin) {
+#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
+  // Allow BrowserOS extensions to access private networks (e.g., localhost).
+  // This enables extension service workers to connect to local servers.
+  if (origin.scheme() == extensions::kExtensionScheme &&
+      browseros::IsActiveBrowserOSExtension(origin.host())) {
+    return LocalNetworkAccessRequestPolicyOverride::kForceAllow;
+  }
+#endif
+
 #if BUILDFLAG(IS_ANDROID)
   if (base::android::device_info::is_automotive()) {
     return content::ContentBrowserClient::
