diff --git a/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.cc b/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.cc
index 64150f8..6629f0e 100644
--- a/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.cc
+++ b/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.cc
@@ -5,8 +5,10 @@
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.h"
 
 #include "base/functional/bind.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/browser_element_identifiers.h"
 #include "chrome/browser/ui/layout_constants.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
 #include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"
 #include "chrome/browser/ui/views/frame/browser_view.h"
 #include "chrome/browser/ui/views/tabs/shared/new_tab_button.h"
@@ -89,10 +91,34 @@ void VerticalTabStripBottomContainer::UpdateButtonStyles(bool collapsed) {
   auto orientation = collapsed ? views::LayoutOrientation::kVertical
                                : views::LayoutOrientation::kHorizontal;
 
+  // 风桥 N3：「+」整宽（展开态铺满标签列 224），36 高、#E8EBEE 底、圆角 10；
+  // 收起时还是一个 32 见方的图标按钮。数值走偏好，改偏好即改外观。
+  using browseros::windrise::LayoutMetric;
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const int new_tab_height =
+      collapsed
+          ? GetLayoutConstant(LayoutConstant::kVerticalTabStripNewTabButtonSize)
+          : metrics.Get(LayoutMetric::kNewTabButtonHeight);
+  new_tab_button_->SetMinSize(gfx::Size(0, new_tab_height));
+  // 展开态：横向排、交叉轴不拉伸，高度取首选尺寸 36（甲06 / 乙02），宽度由下面的
+  // flex 规则铺满标签列；收起态回上游的首选尺寸（32 见方图标按钮）。
+  if (collapsed) {
+    new_tab_button_->SetPreferredSize(std::nullopt);
+  } else {
+    new_tab_button_->SetPreferredSize(
+        gfx::Size(new_tab_height, new_tab_height));
+  }
+  new_tab_button_->SetCornerRadius(
+      static_cast<float>(metrics.Get(LayoutMetric::kTabRadius)));
+  new_tab_button_->SetBackgroundColorId(kColorWindriseTileBackground);
+  new_tab_button_->SetForegroundColorId(kColorTabForegroundActiveFrameActive);
+  new_tab_button_->SetIconSize(metrics.Get(LayoutMetric::kTopButtonIconSize));
+
   // Setting button's layout based on collapsed state
   SetOrientation(orientation);
   SetCrossAxisAlignment(collapsed ? views::LayoutAlignment::kStretch
                                   : views::LayoutAlignment::kStart);
 
   new_tab_button_->SetProperty(
       views::kFlexBehaviorKey,
@@ -102,9 +128,10 @@ void VerticalTabStripBottomContainer::UpdateButtonStyles(bool collapsed) {
                     : views::MaximumFlexSizeRule::kUnbounded,
           false, views::MinimumFlexSizeRule::kPreferred));
 
-  new_tab_button_->SetInsets(GetLayoutInsets(
-      collapsed ? LayoutInset::VERTICAL_TAB_STRIP_BOTTOM_BUTTON_COLLAPSED
-                : LayoutInset::VERTICAL_TAB_STRIP_BOTTOM_BUTTON_UNCOLLAPSED));
+  new_tab_button_->SetInsets(
+      collapsed ? GetLayoutInsets(
+                      LayoutInset::VERTICAL_TAB_STRIP_BOTTOM_BUTTON_COLLAPSED)
+                : gfx::Insets());
 }
 
 BEGIN_METADATA(VerticalTabStripBottomContainer)
