diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_web_view.cc b/chrome/browser/ui/views/frame/windrise_edge_light_web_view.cc
new file mode 100644
index 0000000000..8ea6f36f69
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_web_view.cc
@@ -0,0 +1,131 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/frame/windrise_edge_light_web_view.h"
+
+#include "build/build_config.h"
+#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
+#include "chrome/browser/profiles/profile.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
+#include "content/public/browser/render_widget_host_view.h"
+#include "content/public/browser/web_contents.h"
+#include "extensions/browser/view_type_utils.h"
+#include "extensions/common/mojom/view_type.mojom.h"
+#include "third_party/skia/include/core/SkColor.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/views/accessibility/view_accessibility.h"
+#include "ui/views/controls/native/native_view_host.h"
+#include "ui/views/controls/webview/web_contents_set_background_color.h"
+
+#if defined(USE_AURA)
+#include "ui/aura/window.h"
+#endif
+
+WindriseEdgeLightWebView::WindriseEdgeLightWebView(
+    BrowserWindowInterface* browser)
+    : browser_(browser) {
+  SetBrowserContext(browser->GetProfile());
+  SetVisible(false);
+  // 不进焦点链、不接收任何输入事件、不进无障碍树：它只是一层光。
+  SetFocusBehavior(views::View::FocusBehavior::NEVER);
+  SetCanProcessEventsWithinSubtree(false);
+  GetViewAccessibility().SetIsIgnored(true);
+}
+
+WindriseEdgeLightWebView::~WindriseEdgeLightWebView() {
+  SetWebContents(nullptr);
+}
+
+void WindriseEdgeLightWebView::Revoke() {
+  shown_ = false;
+  SetVisible(false);
+  if (web_contents()) {
+    web_contents()->WasHidden();
+    // WebView detaches its native view and destroys its owned WebContents.
+    SetWebContents(nullptr);
+  }
+  contents_prepared_ = false;
+  loaded_url_ = GURL();
+}
+
+void WindriseEdgeLightWebView::SetShown(bool shown, const GURL& url) {
+  if (!shown || !url.is_valid()) {
+    if (shown_) {
+      shown_ = false;
+      SetVisible(false);
+      if (web_contents()) {
+        web_contents()->WasHidden();
+      }
+    }
+    return;
+  }
+  if (!contents_prepared_) {
+    // 第一次用：建网页视图（WebView 按需建），先挂上扩展身份和透明背景再加载。
+    content::WebContents* contents = GetWebContents();
+    if (!contents) {
+      return;
+    }
+    // 扩展页面要能用 chrome.runtime / chrome.storage（收 worker 广播、读参数）。
+    extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
+        contents);
+    extensions::SetViewType(contents, extensions::mojom::ViewType::kComponent);
+    // 换页 / 渲染进程重建后也保持透明（同 AiOverlayDialogControllerViews）。
+    views::WebContentsSetBackgroundColor::CreateForWebContentsWithColor(
+        contents, SK_ColorTRANSPARENT);
+    contents_prepared_ = true;
+  }
+  if (loaded_url_ != url) {
+    loaded_url_ = url;
+    LoadInitialURL(url);
+  }
+  ApplyTransparencyAndInputPassThrough();
+  if (!shown_) {
+    shown_ = true;
+    SetVisible(true);
+    web_contents()->WasShown();
+  }
+}
+
+void WindriseEdgeLightWebView::SetCornerRadii(
+    const gfx::RoundedCornersF& radii) {
+  if (corner_radii_ == radii) {
+    return;
+  }
+  corner_radii_ = radii;
+  holder()->SetNativeViewCornerRadii(radii);
+}
+
+void WindriseEdgeLightWebView::PrimaryPageChanged(content::Page& page) {
+  views::WebView::PrimaryPageChanged(page);
+  ApplyTransparencyAndInputPassThrough();
+}
+
+void WindriseEdgeLightWebView::RenderViewReady() {
+  ApplyTransparencyAndInputPassThrough();
+}
+
+void WindriseEdgeLightWebView::ApplyTransparencyAndInputPassThrough() {
+  content::WebContents* contents = web_contents();
+  if (!contents) {
+    return;
+  }
+  contents->SetPageBaseBackgroundColor(SK_ColorTRANSPARENT);
+  if (content::RenderWidgetHostView* rwhv =
+          contents->GetRenderWidgetHostView()) {
+    rwhv->SetBackgroundColor(SK_ColorTRANSPARENT);
+  }
+#if defined(USE_AURA)
+  // 原生窗口不做命中目标（连同子窗口）：事件落到下面网页的窗口上。
+  // 和不接输入的 Widget（InitParams::accept_events = false）同一个开关。
+  if (gfx::NativeView native_view = contents->GetNativeView()) {
+    native_view->SetEventTargetingPolicy(aura::EventTargetingPolicy::kNone);
+  }
+#endif
+  // 圆角：网页视图（重）建后 holder 的圆角要再给一次。
+  if (!corner_radii_.IsEmpty()) {
+    holder()->SetNativeViewCornerRadii(corner_radii_);
+  }
+}
+
+BEGIN_METADATA(WindriseEdgeLightWebView)
+END_METADATA
