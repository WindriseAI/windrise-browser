diff --git a/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc b/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc
index c34734562b..6c3cb7f497 100644
--- a/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc
+++ b/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc
@@ -4,10 +4,12 @@
 
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.h"
 
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/ui/actions/chrome_action_id.h"
 #include "chrome/browser/ui/browser_commands.h"
 #include "chrome/browser/ui/browser_element_identifiers.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
 #include "chrome/browser/ui/layout_constants.h"
 #include "chrome/browser/ui/tabs/organizer/organizer_panel_utils.h"
 #include "chrome/browser/ui/tabs/saved_tab_groups/saved_tab_group_utils.h"
@@ -97,6 +99,21 @@ VerticalTabStripTopContainer::~VerticalTabStripTopContainer() = default;
 void VerticalTabStripTopContainer::Layout(PassKey) {
   LayoutSuperclass<views::View>(this);
   combo_button_->SetOrientation(combo_button_orientation_);
+  // 风桥 P1（59c 判定）：格子、搜索和收起按钮一样，各自是一块 #E8EBEE 底、
+  // 圆角 10 的 32 见方 tile（乙01 / 乙02、方案:18、定稿帧 36:26925 与点头页
+  // ntp-zh-rail）。颜色走 mixer 的 kColorWindriseTileBackground，圆角走偏好；
+  // 每次布局给一次（两个 setter 值没变就直接返回），改偏好即改外观。
+  const float top_button_radius = static_cast<float>(
+      browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTopButtonRadius));
+  for (TabStripFlatEdgeButton* button :
+       {combo_button_->start_button(), combo_button_->end_button()}) {
+    if (!button) {
+      continue;
+    }
+    button->SetBackgroundColorId(kColorWindriseTileBackground);
+    button->SetCornerRadius(top_button_radius);
+  }
   combo_button_->SetClipLayerToVisibleBounds(
       !GetLocalBounds().Contains(combo_button_->bounds()));
 }
@@ -186,19 +203,20 @@ views::ProposedLayout VerticalTabStripTopContainer::CalculateProposedLayout(
             combo_button_->GetPreferredSizeForOrientation(
                 combo_button_orientation_);
 
-        const int kComboButtonCollapsedPadding = GetLayoutConstant(
-            LayoutConstant::kVerticalTabStripFlatEdgeButtonPadding);
-
-        // When collapsed the combo button should fill the width (excluding
-        // padding), this is especially relevant when in the expand-on-hover
-        // state.
-        int width =
-            is_collapsed
-                ? (host_size.width() - (2 * kComboButtonCollapsedPadding))
-                : pref_size.width();
-        int padding = is_collapsed
-                          ? kComboButtonCollapsedPadding
-                          : std::max(0, (host_size.width() - width) / 2);
+        // 风桥 P1（59c 判定）：上游收起时把组合按钮撑满栏宽，按钮比 32 宽就
+        // 塞进标签文字，窄栏里露出「…」。这里一律取首选宽（两个 32 见方竖排），
+        // 在收起栏的标签列里水平居中（点头页 ntp-zh-rail：66 栏里居中）。
+        // 悬停展开时栏变宽但仍按收起态排，x 继续按收起栏的宽度算，按钮不跟着
+        // 栏宽往中间滑，和标签图标一样待在原位。
+        const int collapsed_content_width =
+            tabs::kVerticalTabStripCollapsedWidth -
+            2 * GetLayoutConstant(
+                    LayoutConstant::kVerticalTabStripHorizontalPadding);
+        const int width = pref_size.width();
+        const int centering_width =
+            is_collapsed ? std::min(host_size.width(), collapsed_content_width)
+                         : host_size.width();
+        const int padding = std::max(0, (centering_width - width) / 2);
         gfx::Rect bounds(padding, current_y, width, pref_size.height());
         layout.child_layouts.emplace_back(combo_button_.get(),
                                           /*visible=*/true, bounds);
