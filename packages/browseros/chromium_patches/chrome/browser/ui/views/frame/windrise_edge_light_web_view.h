diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_web_view.h b/chrome/browser/ui/views/frame/windrise_edge_light_web_view.h
new file mode 100644
index 0000000000..317bb50b9d
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_web_view.h
@@ -0,0 +1,68 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_WEB_VIEW_H_
+#define CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_WEB_VIEW_H_
+
+#include "base/memory/raw_ptr.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/gfx/geometry/rounded_corners_f.h"
+#include "ui/views/controls/webview/webview.h"
+#include "url/gurl.h"
+
+class BrowserWindowInterface;
+
+namespace content {
+class Page;
+}  // namespace content
+
+// 风桥 「独立层」：AI 操作某个标签时，网页区上方一层透明、不接收任何输入的
+// 网页视图，加载风桥扩展自己的 edge-light/frame.html（和插件页内层、效果图同一份
+// 绘制代码）。换效果只换扩展文件，浏览器不用重编。
+//
+// 先例：上游 ActorOverlayWebView（同一个 ContentsContainerView 里、网页区上方的
+// 透明 WebView，圆角用 holder()->SetNativeViewCornerRadii 跟网页区一致）；透明背景
+// 和扩展身份的写法照 AiOverlayDialogControllerViews / DocumentPipHost。
+// 和 ActorOverlayWebView 不同的是：它要挡住网页的输入，本层一定不能挡——
+// views 一层 SetCanProcessEventsWithinSubtree(false)，aura 一层把网页视图的
+// 原生窗口设成 EventTargetingPolicy::kNone（不接输入的 Widget 用的就是这个），
+// 鼠标、触摸、滚轮都落到下面的网页上；不进焦点链、不进无障碍树。
+//
+// 开 / 收 / 脉冲 / 贴边取样由扩展的 worker 按窗口 id 广播给这一页（地址带
+// #native&w=<窗口 id>）；本类只管显示 / 隐藏、透明、不接输入、圆角。
+// 不在网页的 DOM 里，也不在网页截图（Page.captureScreenshot）里。
+class WindriseEdgeLightWebView : public views::WebView {
+  METADATA_HEADER(WindriseEdgeLightWebView, views::WebView)
+
+ public:
+  explicit WindriseEdgeLightWebView(BrowserWindowInterface* browser);
+  WindriseEdgeLightWebView(const WindriseEdgeLightWebView&) = delete;
+  WindriseEdgeLightWebView& operator=(const WindriseEdgeLightWebView&) = delete;
+  ~WindriseEdgeLightWebView() override;
+
+  // 光效层该不该看得见（含 400ms 淡出）。url 无效（扩展没登记）时一律不显示。
+  // url 变了（扩展换了 / 重装换了 id）就重新加载。
+  void SetShown(bool shown, const GURL& url);
+  bool shown() const { return shown_; }
+  // Drop the owned WebContents and its compositor surface on authority loss.
+  void Revoke();
+
+  // 和网页区同一组四角半径（含 的 AI 过渡）。
+  void SetCornerRadii(const gfx::RoundedCornersF& radii);
+
+  // content::WebContentsObserver:
+  void PrimaryPageChanged(content::Page& page) override;
+  void RenderViewReady() override;
+
+ private:
+  // 透明 + 不接输入；网页视图换了 / 页面换了都要重新设一遍。
+  void ApplyTransparencyAndInputPassThrough();
+
+  raw_ptr<BrowserWindowInterface> browser_;
+  GURL loaded_url_;
+  bool shown_ = false;
+  bool contents_prepared_ = false;
+  gfx::RoundedCornersF corner_radii_;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_WEB_VIEW_H_
