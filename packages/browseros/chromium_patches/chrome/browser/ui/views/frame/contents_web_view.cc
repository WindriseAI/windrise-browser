diff --git a/chrome/browser/ui/views/frame/contents_web_view.cc b/chrome/browser/ui/views/frame/contents_web_view.cc
index 184614e93d..4d4fafc7c2 100644
--- a/chrome/browser/ui/views/frame/contents_web_view.cc
+++ b/chrome/browser/ui/views/frame/contents_web_view.cc
@@ -11,10 +11,16 @@
 #include "chrome/browser/ui/views/frame/safe_invoke/safe_invoke.h"
 #include "chrome/browser/ui/views/frame/web_contents_close_handler.h"
 #include "chrome/browser/ui/views/status_bubble_views.h"
+#include "chrome/common/webui_url_constants.h"
 #include "components/tabs/public/tab_interface.h"
 #include "components/web_modal/web_contents_modal_dialog_manager.h"
+#include "content/public/browser/browser_context.h"
+#include "content/public/browser/navigation_controller.h"
+#include "content/public/browser/navigation_entry.h"
+#include "content/public/browser/render_frame_host.h"
 #include "content/public/browser/render_widget_host_view.h"
 #include "content/public/browser/web_contents.h"
+#include "content/public/common/url_constants.h"
 #include "third_party/blink/public/common/input/web_input_event.h"
 #include "third_party/skia/include/core/SkColor.h"
 #include "ui/base/metadata/metadata_impl_macros.h"
@@ -23,6 +29,7 @@
 #include "ui/compositor/layer_tree_owner.h"
 #include "ui/views/view_class_properties.h"
 #include "ui/views/widget/widget.h"
+#include "url/gurl.h"
 
 #if defined(USE_AURA)
 #include "ui/aura/window.h"
@@ -36,6 +43,43 @@
 DEFINE_CLASS_ELEMENT_IDENTIFIER_VALUE(ContentsWebView,
                                       kContentsWebViewElementId);
 
+namespace {
+
+// Windrise new tab page: newtab.html of the Windrise extension (same id as
+// kAssistantId in windrise_extension_protection.cc). Its page background is
+// `html.ntp { background: #B9C0C7 }` in ntp.css, in light and dark mode, and
+// the background image fades in over it. Showing the same color before the
+// page's first frame replaces the white flash on Ctrl+T / "+".
+constexpr char kWindriseNewTabExtensionHost[] =
+    "amlnokjigkbgngdgkofikfphjbknfodp";
+constexpr char kWindriseNewTabPath[] = "/newtab.html";
+constexpr SkColor kWindriseNewTabBackground = SkColorSetRGB(0xB9, 0xC0, 0xC7);
+
+bool IsWindriseNewTabUrl(const GURL& url) {
+  if (url.SchemeIs(content::kChromeUIScheme)) {
+    // chrome://newtab before it is rewritten to the extension page.
+    return url.host() == chrome::kChromeUINewTabHost;
+  }
+  return url.SchemeIs("chrome-extension") &&
+         url.host() == kWindriseNewTabExtensionHost &&
+         url.path() == kWindriseNewTabPath;
+}
+
+bool IsWindriseNewTabPage(content::WebContents* web_contents) {
+  if (!web_contents ||
+      (web_contents->GetBrowserContext() &&
+       web_contents->GetBrowserContext()->IsOffTheRecord())) {
+    return false;
+  }
+  content::NavigationController& controller = web_contents->GetController();
+  const content::NavigationEntry* entry = controller.GetPendingEntry()
+                                              ? controller.GetPendingEntry()
+                                              : controller.GetLastCommittedEntry();
+  return entry && IsWindriseNewTabUrl(entry->GetURL());
+}
+
+}  // namespace
+
 ContentsWebView::ContentsWebView(content::BrowserContext* browser_context)
     : views::WebView(browser_context),
       use_default_deadline_when_animating_(
@@ -167,6 +211,13 @@ void ContentsWebView::SetWebContents(content::WebContents* web_contents) {
     return;
   }
 
+  // Windrise: a freshly created tab has no live renderer yet, so
+  // RenderViewReady() has not colored it; without this its first frames are
+  // the RenderWidgetHostView's white fallback.
+  if (GetWidget()) {
+    UpdateBackgroundColor();
+  }
+
   UpdateIsBlockedByModal();
 
   if (!status_bubble_) {
@@ -208,9 +259,12 @@ void ContentsWebView::DidGetUserInteraction(const blink::WebInputEvent& event) {
 #endif
 
 void ContentsWebView::UpdateBackgroundColor() {
-  const SkColor color = GetColorProvider()->GetColor(
-      is_letterboxing() ? kColorWebContentsBackgroundLetterboxing
-                        : kColorWebContentsBackground);
+  const SkColor color =
+      !is_letterboxing() && IsWindriseNewTabPage(web_contents())
+          ? kWindriseNewTabBackground
+          : GetColorProvider()->GetColor(
+                is_letterboxing() ? kColorWebContentsBackgroundLetterboxing
+                                  : kColorWebContentsBackground);
   // `color` must be opaque, see RenderWidgetHostView::SetBackgroundColor() for
   // details.
   // TODO(crbug.com/456309057): Update this to be a CHECK.
@@ -286,6 +340,16 @@ void ContentsWebView::DestroyClonedLayer() {
   cloned_layer_tree_.reset();
 }
 
+void ContentsWebView::RenderFrameHostChanged(
+    content::RenderFrameHost* old_host,
+    content::RenderFrameHost* new_host) {
+  views::WebView::RenderFrameHostChanged(old_host, new_host);
+  if (GetWidget() && web_contents() &&
+      new_host == web_contents()->GetPrimaryMainFrame()) {
+    UpdateBackgroundColor();
+  }
+}
+
 void ContentsWebView::RenderViewReady() {
   // Set the background color to be the theme's ntp background on startup.
   if (GetWidget()) {
