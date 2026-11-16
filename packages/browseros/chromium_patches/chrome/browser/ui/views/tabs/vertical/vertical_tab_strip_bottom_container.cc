diff --git a/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.cc b/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.cc
index 64150f80cf..abfee1317f 100644
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
@@ -89,6 +91,25 @@ void VerticalTabStripBottomContainer::UpdateButtonStyles(bool collapsed) {
   auto orientation = collapsed ? views::LayoutOrientation::kVertical
                                : views::LayoutOrientation::kHorizontal;
 
+  // 风桥 N3：「+」整宽（展开态铺满标签列 224），36 高、#E8EBEE 底、圆角 10；
+  // 收起时同样 36 高、宽同收起态标签（P3 / Q4：点头页 ntp-zh-rail 收起态「+」
+  // 54×36，和标签同宽同高）。数值走偏好，改偏好即改外观。
+  using browseros::windrise::LayoutMetric;
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const int new_tab_height = metrics.Get(LayoutMetric::kNewTabButtonHeight);
+  new_tab_button_->SetMinSize(gfx::Size(0, new_tab_height));
+  // 高度取首选尺寸 36（甲06 / 乙02）：展开态横向排、交叉轴不拉伸，宽度由下面的
+  // flex 规则铺满标签列；收起态竖向排、主轴按首选高 36，交叉轴拉伸成标签宽。
+  // 收起态不能清掉首选尺寸：TabStripFlatEdgeButton 的默认首选是 32 见方，主轴
+  // 只认首选高，清掉就回到 32。
+  new_tab_button_->SetPreferredSize(gfx::Size(new_tab_height, new_tab_height));
+  new_tab_button_->SetCornerRadius(
+      static_cast<float>(metrics.Get(LayoutMetric::kTabRadius)));
+  new_tab_button_->SetBackgroundColorId(kColorWindriseTileBackground);
+  new_tab_button_->SetForegroundColorId(kColorTabForegroundActiveFrameActive);
+  new_tab_button_->SetIconSize(metrics.Get(LayoutMetric::kTopButtonIconSize));
+
   // Setting button's layout based on collapsed state
   SetOrientation(orientation);
   SetCrossAxisAlignment(collapsed ? views::LayoutAlignment::kStretch
@@ -102,9 +123,10 @@ void VerticalTabStripBottomContainer::UpdateButtonStyles(bool collapsed) {
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
