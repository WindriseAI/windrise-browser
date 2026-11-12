diff --git a/chrome/browser/ui/layout_constants.cc b/chrome/browser/ui/layout_constants.cc
index b8206dc..b369164 100644
--- a/chrome/browser/ui/layout_constants.cc
+++ b/chrome/browser/ui/layout_constants.cc
@@ -6,6 +6,7 @@
 
 #include "base/notreached.h"
 #include "build/build_config.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/tabs/features.h"
 #include "chrome/browser/ui/ui_features.h"
 #include "components/omnibox/common/omnibox_features.h"
@@ -157,16 +158,23 @@ int GetLayoutConstant(LayoutConstant constant) {
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
@@ -174,7 +182,9 @@ int GetLayoutConstant(LayoutConstant constant) {
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
@@ -184,9 +194,13 @@ int GetLayoutConstant(LayoutConstant constant) {
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
