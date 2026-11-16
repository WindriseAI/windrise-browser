diff --git a/chrome/browser/ui/views/frame/browser_frame_view_win.cc b/chrome/browser/ui/views/frame/browser_frame_view_win.cc
index 8bcc77c6b9..9ebca73cc9 100644
--- a/chrome/browser/ui/views/frame/browser_frame_view_win.cc
+++ b/chrome/browser/ui/views/frame/browser_frame_view_win.cc
@@ -16,6 +16,7 @@
 #include "base/trace_event/trace_event.h"
 #include "chrome/app/chrome_command_ids.h"
 #include "chrome/app/chrome_dll_resource.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/themes/theme_properties.h"
 #include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
 #include "chrome/browser/ui/color/chrome_color_id.h"
@@ -734,11 +735,29 @@ void BrowserFrameViewWin::LayoutCaptionButtons() {
   const gfx::Size preferred_size =
       caption_button_container_->GetPreferredSize();
 
-  const int height =
+  int height =
       !GetBrowserView()->GetWebAppFrameToolbarPreferredSize().IsEmpty()
           ? (TitlebarHeight(false) - WindowTopY())
           : GetFrameHeight();
 
+  // 风桥 甲23 §4.4-b：竖向标签栏的普通窗口里，标题按钮区的下沿和工具栏下沿
+  // 对齐。上游高 = 工具栏最小高 + 2，从 WindowTopY()（还原窗口为 1）开始，
+  // 底边比工具栏多 1 行：它会把工具栏撑到 49；工具栏改回 48 后又会压住书签栏 /
+  // 网页第一行，悬停「关闭」时露出 1px 红，点击区也多一行。这里按
+  // BrowserViewTabbedLayoutImpl 里工具栏的同一个式子（max(偏好工具栏高,
+  // 工具栏最小高)）算下沿 = 客户区顶 + 工具栏高。点击区就是这个容器的范围，
+  // 一起跟着走。
+  if (GetBrowserView()->ShouldDrawVerticalTabStrip() &&
+      GetBrowserView()->GetWebAppFrameToolbarPreferredSize().IsEmpty() &&
+      !GetBrowserView()->IsWindowControlsOverlayEnabled() &&
+      !browser_widget()->IsFullscreen()) {
+    const int toolbar_height =
+        std::max(browseros::windrise::GetLayoutMetrics().Get(
+                     browseros::windrise::LayoutMetric::kToolbarHeight),
+                 GetClientFrameElementInfo().toolbar_minimum_height);
+    height = std::max(0, GetTopInset(false) + toolbar_height - WindowTopY());
+  }
+
   caption_button_container_->SetBounds(width() - preferred_size.width(),
                                        WindowTopY(), preferred_size.width(),
                                        height);
