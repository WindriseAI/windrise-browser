diff --git a/chrome/browser/ui/views/chrome_layout_provider.cc b/chrome/browser/ui/views/chrome_layout_provider.cc
index 6ee376f2de..3b4a41d343 100644
--- a/chrome/browser/ui/views/chrome_layout_provider.cc
+++ b/chrome/browser/ui/views/chrome_layout_provider.cc
@@ -8,6 +8,7 @@
 
 #include "base/containers/fixed_flat_map.h"
 #include "base/feature_list.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/ui_features.h"
 #include "chrome/browser/ui/views/chrome_typography.h"
 #include "components/omnibox/common/omnibox_features.h"
@@ -351,6 +352,22 @@ int ChromeLayoutProvider::GetCornerRadiusMetric(views::ShapeContextToken token,
   CHECK_GE(token, views::kShapeContextTokenStart, base::NotFatalUntil::M162);
   CHECK_LT(token, kChromeShapeContextTokenEnd, base::NotFatalUntil::M162);
 
+  // 风桥：弹层圆角走偏好（方案 N2，偏好 browseros.windrise.layout.radius_*）。
+  // 菜单、气泡、对话框在 158 里都取这几个 token，先统一取 radius_medium(12)。
+  // 方案里对话框要 16，但 158 的对话框和气泡共用 kDialogRadius，没法定点区分，
+  // 见报告「没把握 / 待办」。
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  switch (token) {
+    case views::kDialogRadius:
+    case views::kMenuRadius:
+    case views::kMenuAuxRadius:
+    case views::kMenuTouchRadius:
+      return metrics.Get(browseros::windrise::LayoutMetric::kRadiusMedium);
+    default:
+      break;
+  }
+
   if (token < views::ShapeContextTokens::kShapeContextTokenEnd) {
     return LayoutProvider::GetCornerRadiusMetric(token, size);
   }
