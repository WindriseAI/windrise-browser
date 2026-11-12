diff --git a/chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc b/chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc
index b1ea4a6..b9daa15 100644
--- a/chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc
+++ b/chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc
@@ -15,6 +15,7 @@
 #include "base/trace_event/common/trace_event_common.h"
 #include "base/trace_event/trace_event.h"
 #include "build/build_config.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/animation/browser_animation_controller.h"
 #include "chrome/browser/ui/animation/browser_animation_types.h"
 #include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
@@ -67,7 +68,7 @@
 namespace {
 
 // Minimum area next to caption buttons to use as a grab handle.
-constexpr int kVerticalTabsGrabHandleSize = 40;
+constexpr int kVerticalTabsGrabHandleSize = 5;
 
 // Maximum portion of the window a "size-restricted" contents-height side panel
 // can take up. This is not the only limit on side panel size.
@@ -1586,11 +1587,20 @@ void BrowserViewTabbedLayoutImpl::DoPreLayoutComputations(
 
 void BrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(
     const BrowserLayoutParams& params) {
-  // If the window goes out of glass mode, these changes won't hurt anything.
-  if (in_glass_mode()) {
-    gfx::RoundedCornersF content_corners;
-    if (layout_data_->tab_strip_type == TabStripType::kVertical &&
-        !is_fullscreen(layout_data_->window_state)) {
+  // 风桥 N10：竖向标签栏下，网页区上两角圆角（偏好
+  // browseros.windrise.layout.web_contents_radius，默认 12），四边缝 0；全屏 /
+  // 网页全屏时圆角回 0。下两角跟窗口自身的圆角（Windows 11 由系统裁），所以
+  // 这里只在下边沿用上游逻辑给一个 leading corner。
+  gfx::RoundedCornersF content_corners;
+  const bool vertical_and_windowed =
+      layout_data_->tab_strip_type == TabStripType::kVertical &&
+      !is_fullscreen(layout_data_->window_state);
+  if (vertical_and_windowed) {
+    const int web_contents_radius = browseros::windrise::GetLayoutMetrics().Get(
+        browseros::windrise::LayoutMetric::kWebContentsRadius);
+    content_corners.set_upper_left(web_contents_radius);
+    content_corners.set_upper_right(web_contents_radius);
+    if (in_glass_mode()) {
       // Note that this will set a lower leading corner on the multi contents
       // view even if there's a shadow box, but since the curve is effectively
       // the same this will not produce a visual bug.
@@ -1602,8 +1612,8 @@ void BrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(
         content_corners.set_lower_left(radius);
       }
     }
-    views().multi_contents_view->SetBackgroundRadii(content_corners);
   }
+  views().multi_contents_view->SetBackgroundRadii(content_corners);
 
   // Want to cut the vertical tabstrip and its decorations out of the top
   // container in transparency mode.
