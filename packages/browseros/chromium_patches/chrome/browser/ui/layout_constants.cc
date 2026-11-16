diff --git a/chrome/browser/ui/layout_constants.cc b/chrome/browser/ui/layout_constants.cc
index b8206dc188..a24c579b62 100644
--- a/chrome/browser/ui/layout_constants.cc
+++ b/chrome/browser/ui/layout_constants.cc
@@ -4,8 +4,11 @@
 
 #include "chrome/browser/ui/layout_constants.h"
 
+#include <algorithm>
+
 #include "base/notreached.h"
 #include "build/build_config.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/tabs/features.h"
 #include "chrome/browser/ui/ui_features.h"
 #include "components/omnibox/common/omnibox_features.h"
@@ -27,11 +30,19 @@ int GetLayoutConstant(LayoutConstant constant) {
     case LayoutConstant::kAppMenuButtonImageLabelPadding:
       return features::IsToolbarGlowUpEnabled() ? 4 : 2;
     case LayoutConstant::kBookmarkBarHeight: {
-      // The fixed margin ensures the bookmark buttons appear centered relative
-      // to the white space above and below.
-      const int bookmark_bar_attached_vertical_margin = 6;
-      return GetLayoutConstant(LayoutConstant::kBookmarkBarButtonHeight) +
-             bookmark_bar_attached_vertical_margin;
+      const int button_height =
+          GetLayoutConstant(LayoutConstant::kBookmarkBarButtonHeight);
+      if (touch_ui) {
+        // The fixed margin ensures the bookmark buttons appear centered
+        // relative to the white space above and below.
+        const int bookmark_bar_attached_vertical_margin = 6;
+        return button_height + bookmark_bar_attached_vertical_margin;
+      }
+      // 风桥 甲23：书签栏高走偏好（默认 32，乙02）；按钮 28 在栏内居中，上下各 2
+      // （见 BookmarkBarView::Layout）。不低于按钮高，免得按钮被裁。
+      return std::max(button_height,
+                      browseros::windrise::GetLayoutMetrics().Get(
+                          browseros::windrise::LayoutMetric::kBookmarkBarHeight));
     }
     case LayoutConstant::kBookmarkBarButtonHeight:
       return touch_ui ? 36 : 28;
@@ -157,16 +168,23 @@ int GetLayoutConstant(LayoutConstant constant) {
       return 20;
     case LayoutConstant::kMainBackgroundRegionCornerRadius:
     case LayoutConstant::kToolbarCornerRadius:
-    case LayoutConstant::kVerticalTabCornerRadius:
       return 8;
+    case LayoutConstant::kVerticalTabCornerRadius:
+      // 风桥 N3：标签圆角走偏好（默认 10，方案 3 节）。
+      return browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTabRadius);
     case LayoutConstant::kVerticalTabHeight:
-      return 30;
+      // 风桥 N3：标签高走偏好（默认 36）。
+      return browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTabHeight);
     case LayoutConstant::kVerticalTabPinnedHeight:
       return 32;
     case LayoutConstant::kVerticalTabMinWidth:
       return 32;
     case LayoutConstant::kVerticalTabStripHorizontalPadding:
-      return 12;
+      // 风桥 N3：标签列左右各留 8（240 宽的栏里标签 224）。
+      return browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTabStripHorizontalPadding);
     case LayoutConstant::kVerticalTabStripUncollapsedVerticalPadding:
       return 12;
     case LayoutConstant::kVerticalTabStripCollapsedVerticalPadding:
@@ -174,7 +192,9 @@ int GetLayoutConstant(LayoutConstant constant) {
     case LayoutConstant::kVerticalTabStripComboButtonIconSize:
       return 18;
     case LayoutConstant::kVerticalTabStripButtonIconSize:
-      return 20;
+      // 风桥 N3：顶部小按钮图标 16。
+      return browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTopButtonIconSize);
     case LayoutConstant::kVerticalTabStripTopButtonPadding:
       return 4;
     case LayoutConstant::kVerticalTabStripFlatEdgeButtonPadding:
@@ -184,9 +204,13 @@ int GetLayoutConstant(LayoutConstant constant) {
     case LayoutConstant::kVerticalTabStripNewTabButtonSize:
       return 32;
     case LayoutConstant::kVerticalTabStripCollapseButtonSize:
-      return 32;
+      // 风桥 N3：展开收起按钮 32 见方（与顶部其它按钮同高）。
+      return browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTopButtonSize);
     case LayoutConstant::kVerticalTabStripTopContainerButtonSize:
-      return 28;
+      // 风桥 N3：顶部小按钮 32 见方。
+      return browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTopButtonSize);
     case LayoutConstant::kVerticalTabPinnedBorderThickness:
       return 1;
     default:
