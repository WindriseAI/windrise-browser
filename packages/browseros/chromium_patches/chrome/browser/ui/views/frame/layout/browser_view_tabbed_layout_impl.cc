diff --git a/chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc b/chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc
index b1ea4a69f4..ca4958b387 100644
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
@@ -1018,8 +1019,11 @@ BrowserViewTabbedLayoutImpl::CalculateProposedLayout(
       corner_bounds.Outset(
           gfx::Outsets::TLBR(0, views::Separator::kThickness, 0, 0));
     }
+    // 风桥 甲23 §4.4：定稿帧 36:26925 里侧栏和工具栏之间是一条直的 .5px 线，
+    // 工具栏左上角是直角（x=241–242、y=0–6 全是工具栏色）。上游在这里放一个
+    // 带描边的内凹圆角（实物 x=247–252、y=0–8 那段弧），风桥不显示。
     layout.AddChild(views().vertical_tab_strip_top_corner, corner_bounds,
-                    top_corner_visible);
+                    /*visible=*/false);
   }
 
   // Position the vertical tabstrip bottom corner.
@@ -1465,6 +1469,17 @@ gfx::Rect BrowserViewTabbedLayoutImpl::CalculateTopContainerLayout(
                           params.visual_client_area.y(),
                           params.visual_client_area.width(),
                           views().toolbar->GetPreferredSize().height());
+      if (layout_data_->tab_strip_type == TabStripType::kVertical) {
+        // 风桥 甲23 §4.4-b：竖向标签栏下工具栏高固定走偏好（默认 48，乙02），
+        // 不再被右侧标题按钮区撑高（上游这里是 max(排除区高, 最小高)，Windows
+        // 还原窗口上排除区底边在 49）。不低于工具栏最小高，免得内容被裁。
+        // BrowserFrameViewWin::LayoutCaptionButtons 用同一个式子定标题按钮区
+        // 的底边，两者始终对齐。
+        toolbar_bounds.set_height(std::max(
+            browseros::windrise::GetLayoutMetrics().Get(
+                browseros::windrise::LayoutMetric::kToolbarHeight),
+            views().toolbar->GetMinimumSize().height()));
+      }
       params.SetTop(toolbar_bounds.bottom());
       needs_exclusion = false;
     }
@@ -1586,11 +1601,22 @@ void BrowserViewTabbedLayoutImpl::DoPreLayoutComputations(
 
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
+  // 这是「基准」半径；AI 操作时 12 -> 36 的过渡（逐角，300ms）叠加在
+  // ContentsContainerView 里，不在这里改。
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
@@ -1602,8 +1628,8 @@ void BrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(
         content_corners.set_lower_left(radius);
       }
     }
-    views().multi_contents_view->SetBackgroundRadii(content_corners);
   }
+  views().multi_contents_view->SetBackgroundRadii(content_corners);
 
   // Want to cut the vertical tabstrip and its decorations out of the top
   // container in transparency mode.
@@ -1706,11 +1732,18 @@ void BrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(
 
     CustomCornersBackground::Outline vertical_tabs_outline;
     vertical_tabs_outline.color = {
-        .color = kColorVerticalTabStripShadow,
+        // 风桥 甲23 §4.4：侧栏右沿的线用分隔线色 #C9CED5（kLine），见下面的 0.5。
+        // 上游 kColorVerticalTabStripShadow = 中性描边色 60%，实物读到
+        // (207,209,211)，不是规范里的线色。
+        .color = kColorToolbarContentAreaSeparator,
         // Vertical tabs outline fades partially during expand-on-hover to be
         // replaced with shadow. Note that the glass alpha will affect the
         // entire paint operation, so no need to re-multiply it in here.
+        // 风桥：再乘 0.5，1 倍屏上这 1px 就是 .5px 的观感（与网页上沿那条一致）。
+        // 这里是 DIP 描边、不认倍数：2 倍屏上是 2 个设备像素的 50%，墨量等于
+        // .5px 实线，但不是「1 个设备像素实色」。
         .opacity =
+            0.5f *
             static_cast<float>(1.0f - kVerticalTabStripOutlineFadeOnHover *
                                           vertical_tabs_bottom_corner_amount) *
             frame_color.opacity,
@@ -1727,6 +1760,15 @@ void BrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(
       vertical_tabs_outline.bottom = true;
     }
     vertical_tabs_background->SetOutline(vertical_tabs_outline);
+    // 风桥 甲23 §4.4：网页左下角那个内凹圆角（上游 kBottomCorner 默认 1.0，平时
+    // 可见；网页角要圆是用户多次说过的，保留）的描边原来是上游的
+    // kColorVerticalTabStripShadow，和上面改过的侧栏右沿线接不上色。改成和右沿
+    // 同色同透明度，这条线一直到底都是同一种线。
+    if (CustomFloatingCorner* const bottom_corner_view =
+            views().vertical_tab_strip_bottom_corner.get()) {
+      bottom_corner_view->SetStroke(vertical_tabs_outline.color,
+                                    /*is_vertical_window_edge=*/true);
+    }
 
     // Cut the vertical tab strip out of the top container.
     if (!frame_color.is_opaque()) {
