diff --git a/chrome/browser/ui/views/tabs/common/vertical_tab_style_views.cc b/chrome/browser/ui/views/tabs/common/vertical_tab_style_views.cc
index dd7a0a8..7d8fbe9 100644
--- a/chrome/browser/ui/views/tabs/common/vertical_tab_style_views.cc
+++ b/chrome/browser/ui/views/tabs/common/vertical_tab_style_views.cc
@@ -7,6 +7,7 @@
 #include <optional>
 #include <string>
 
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/themes/theme_properties.h"
 #include "chrome/browser/ui/layout_constants.h"
 #include "chrome/browser/ui/views/frame/browser_frame_view.h"
@@ -225,9 +226,14 @@ SkScalar VerticalTabStyleViews::GetCornerRadius() const {
 }
 
 gfx::Insets VerticalTabStyleViews::GetContentsInsets() const {
-  return gfx::Insets::VH(
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  // 风桥 N3：标签内左 10 / 右 12（方案标注表第 3 行），垂直方向沿用上游。
+  return gfx::Insets::TLBR(
       GetLayoutConstant(LayoutConstant::kTabVerticalPadding),
-      GetLayoutConstant(LayoutConstant::kTabHorizontalPadding));
+      metrics.Get(browseros::windrise::LayoutMetric::kTabContentsInsetLeft),
+      GetLayoutConstant(LayoutConstant::kTabVerticalPadding),
+      metrics.Get(browseros::windrise::LayoutMetric::kTabContentsInsetRight));
 }
 
 int VerticalTabStyleViews::GetStrokeThickness() const {
