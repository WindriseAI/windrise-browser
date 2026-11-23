diff --git a/chrome/browser/ui/views/frame/contents_web_view.cc b/chrome/browser/ui/views/frame/contents_web_view.cc
index 184614e..581f990 100644
--- a/chrome/browser/ui/views/frame/contents_web_view.cc
+++ b/chrome/browser/ui/views/frame/contents_web_view.cc
@@ -11,18 +11,26 @@
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
 #include "ui/color/color_provider.h"
 #include "ui/compositor/layer_solid_color.h"
 #include "ui/compositor/layer_tree_owner.h"
+#include "ui/gfx/color_utils.h"
 #include "ui/views/view_class_properties.h"
 #include "ui/views/widget/widget.h"
+#include "url/gurl.h"
 
 #if defined(USE_AURA)
 #include "ui/aura/window.h"
@@ -36,6 +44,71 @@
 DEFINE_CLASS_ELEMENT_IDENTIFIER_VALUE(ContentsWebView,
                                       kContentsWebViewElementId);
 
+namespace {
+
+// Windrise new tab page: newtab.html of the Windrise extension (same id as
+// kAssistantId in windrise_extension_protection.cc). Its page background in
+// ntp.css is #B9C0C7 in light mode and #2E343B in dark mode, and the
+// background image fades in over it. Showing the matching color before the
+// page's first frame replaces the flash on Ctrl+T / "+".
+constexpr char kWindriseNewTabExtensionHost[] =
+    "amlnokjigkbgngdgkofikfphjbknfodp";
+constexpr char kWindriseNewTabPath[] = "/newtab.html";
+constexpr SkColor kWindriseNewTabBackgroundLight =
+    SkColorSetRGB(0xB9, 0xC0, 0xC7);
+constexpr SkColor kWindriseNewTabBackgroundDark =
+    SkColorSetRGB(0x2E, 0x34, 0x3B);
+
+bool IsWindriseNewTabUrl(const GURL& url) {
+  if (url.is_empty() || url.IsAboutBlank()) {
+    return true;
+  }
+  if (url.SchemeIs(content::kChromeUIScheme)) {
+    // chrome://newtab and chrome://new-tab-page before rewrite or fallback.
+    return url.host() == chrome::kChromeUINewTabHost ||
+           url.host() == chrome::kChromeUINewTabPageHost ||
+           url.host() == chrome::kChromeUINewTabPageThirdPartyHost;
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
+  if (!entry) {
+    // Brand-new tab/window before any navigation entry commits.
+    return true;
+  }
+  return IsWindriseNewTabUrl(entry->GetURL());
+}
+
+bool IsChromeInternalPage(content::WebContents* web_contents) {
+  if (!web_contents) {
+    return false;
+  }
+  content::NavigationController& controller = web_contents->GetController();
+  const content::NavigationEntry* entry = controller.GetPendingEntry()
+                                              ? controller.GetPendingEntry()
+                                              : controller.GetLastCommittedEntry();
+  return entry && entry->GetURL().SchemeIs(content::kChromeUIScheme);
+}
+
+SkColor GetWindriseNewTabPrePaintColor(SkColor default_color) {
+  return color_utils::IsDark(default_color) ? kWindriseNewTabBackgroundDark
+                                            : kWindriseNewTabBackgroundLight;
+}
+
+}  // namespace
+
 ContentsWebView::ContentsWebView(content::BrowserContext* browser_context)
     : views::WebView(browser_context),
       use_default_deadline_when_animating_(
@@ -167,6 +240,13 @@ void ContentsWebView::SetWebContents(content::WebContents* web_contents) {
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
@@ -181,6 +261,13 @@ void ContentsWebView::SetWebContents(content::WebContents* web_contents) {
       .Then(&web_modal::WebContentsModalDialogManager::UpdateDialogHost);
 }
 
+void ContentsWebView::AddedToWidget() {
+  views::WebView::AddedToWidget();
+  if (GetWidget()) {
+    UpdateBackgroundColor();
+  }
+}
+
 #if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
 void ContentsWebView::DidGetUserInteraction(const blink::WebInputEvent& event) {
   // If the user interacts with the web contents, ensure it is activated.
@@ -208,9 +295,14 @@ void ContentsWebView::DidGetUserInteraction(const blink::WebInputEvent& event) {
 #endif
 
 void ContentsWebView::UpdateBackgroundColor() {
-  const SkColor color = GetColorProvider()->GetColor(
+  const SkColor default_color = GetColorProvider()->GetColor(
       is_letterboxing() ? kColorWebContentsBackgroundLetterboxing
                         : kColorWebContentsBackground);
+  const bool is_windrise_ntp =
+      !is_letterboxing() && IsWindriseNewTabPage(web_contents());
+  const SkColor color = is_windrise_ntp
+                            ? GetWindriseNewTabPrePaintColor(default_color)
+                            : default_color;
   // `color` must be opaque, see RenderWidgetHostView::SetBackgroundColor() for
   // details.
   // TODO(crbug.com/456309057): Update this to be a CHECK.
@@ -229,6 +321,20 @@ void ContentsWebView::UpdateBackgroundColor() {
       .Then(&content::WebContents::GetRenderWidgetHostView)
       .Then(&content::RenderWidgetHostView::SetBackgroundColor,
             background_visible_ ? color : SK_ColorTRANSPARENT);
+
+  // Set Blink page base background for Windrise NTP / about:blank / chrome://
+  // internal pages so first-paint frames never flash white or #202124 black,
+  // while leaving regular websites untouched (std::nullopt).
+  if (content::WebContents* contents = web_contents()) {
+    if (background_visible_ && is_windrise_ntp) {
+      contents->SetPageBaseBackgroundColor(color);
+    } else if (background_visible_ && !is_letterboxing() &&
+               IsChromeInternalPage(contents)) {
+      contents->SetPageBaseBackgroundColor(default_color);
+    } else {
+      contents->SetPageBaseBackgroundColor(std::nullopt);
+    }
+  }
 }
 
 std::unique_ptr<ui::Layer> ContentsWebView::RecreateLayer() {
@@ -286,6 +392,16 @@ void ContentsWebView::DestroyClonedLayer() {
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
