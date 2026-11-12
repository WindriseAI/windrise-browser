diff --git a/chrome/browser/ui/views/chrome_layout_provider.cc b/chrome/browser/ui/views/chrome_layout_provider.cc
index 6ee376f..9671b65 100644
--- a/chrome/browser/ui/views/chrome_layout_provider.cc
+++ b/chrome/browser/ui/views/chrome_layout_provider.cc
@@ -8,6 +8,7 @@
 
 #include "base/containers/fixed_flat_map.h"
 #include "base/feature_list.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/ui_features.h"
 #include "chrome/browser/ui/views/chrome_typography.h"
 #include "components/omnibox/common/omnibox_features.h"
@@ -351,6 +352,29 @@ int ChromeLayoutProvider::GetCornerRadiusMetric(views::ShapeContextToken token,
   CHECK_GE(token, views::kShapeContextTokenStart, base::NotFatalUntil::M162);
   CHECK_LT(token, kChromeShapeContextTokenEnd, base::NotFatalUntil::M162);
 
+  // 风桥：弹层 / 控件圆角走偏好（方案 N2，偏好 browseros.windrise.layout.radius_*）。
+  // - 对话框 16（radius_large）；
+  // - 菜单 / 气泡 12（radius_medium）。气泡在 BubbleDialogDelegate 构造函数里
+  //   把 corner_radius 直接设成 kMenuRadius，所以两者不再共用 kDialogRadius；
+  // - 输入框 / 下拉 / 徽标 8（radius_small）。
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  switch (token) {
+    case views::kDialogRadius:
+      return metrics.Get(browseros::windrise::LayoutMetric::kRadiusLarge);
+    case views::kMenuRadius:
+    case views::kMenuAuxRadius:
+    case views::kMenuTouchRadius:
+      return metrics.Get(browseros::windrise::LayoutMetric::kRadiusMedium);
+    case views::kBadgeRadius:
+    case views::kComboboxRadius:
+    case views::kTextfieldRadius:
+    case views::kContentSeparatorRadius:
+      return metrics.Get(browseros::windrise::LayoutMetric::kRadiusSmall);
+    default:
+      break;
+  }
+
   if (token < views::ShapeContextTokens::kShapeContextTokenEnd) {
     return LayoutProvider::GetCornerRadiusMetric(token, size);
   }
