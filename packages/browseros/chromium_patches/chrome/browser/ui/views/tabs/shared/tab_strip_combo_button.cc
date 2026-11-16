diff --git a/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc b/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc
index 0f3e88328c..c3e5e21ec8 100644
--- a/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc
+++ b/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc
@@ -34,6 +34,7 @@
 #include "ui/base/l10n/l10n_util.h"
 #include "ui/base/metadata/metadata_impl_macros.h"
 #include "ui/base/ui_base_features.h"
+#include "ui/gfx/animation/tween.h"
 #include "ui/views/actions/action_view_controller.h"
 #include "ui/views/controls/button/menu_button_controller.h"
 #include "ui/views/controls/menu/menu_model_adapter.h"
@@ -183,6 +184,48 @@ void TabStripComboButton::SetOrientation(views::LayoutOrientation orientation) {
   UpdateStyles();
 }
 
+void TabStripComboButton::SetOrientationTransitionProgress(double progress) {
+  progress = std::clamp(progress, 0.0, 1.0);
+  if (orientation_transition_progress_ == progress) {
+    return;
+  }
+  orientation_transition_progress_ = progress;
+  InvalidateLayout();
+}
+
+void TabStripComboButton::Layout(PassKey) {
+  LayoutSuperclass<views::View>(this);
+
+  // 风桥 进度在 (0, 1) 之间时，每个按钮从竖排位置
+  // 朝横排位置线性插值，不先藏再出、也不在半路跳到另一种排法。尺寸取首选
+  // 尺寸（32 见方），跳过规则和 GetPreferredSizeForOrientation 一样，所以
+  // 两端和 BoxLayout 排出来的位置逐像素相同。进度本身由顶部容器按栏此刻
+  // 的宽度给，时长、曲线、打断都跟着侧栏的悬停展开走。
+  const double progress = orientation_transition_progress_;
+  if (progress <= 0.0 || progress >= 1.0) {
+    return;
+  }
+  const int spacing =
+      GetLayoutConstant(LayoutConstant::kVerticalTabStripFlatEdgeButtonPadding);
+  int vertical_y = 0;
+  int horizontal_x = 0;
+  for (views::View* child : children()) {
+    if (!child->GetVisible()) {
+      continue;
+    }
+    const gfx::Size child_size = child->GetPreferredSize();
+    if (child_size.width() == 0) {
+      continue;
+    }
+    child->SetBoundsRect(gfx::Rect(
+        gfx::Tween::LinearIntValueBetween(progress, 0, horizontal_x),
+        gfx::Tween::LinearIntValueBetween(progress, vertical_y, 0),
+        child_size.width(), child_size.height()));
+    vertical_y += child_size.height() + spacing;
+    horizontal_x += child_size.width() + spacing;
+  }
+}
+
 void TabStripComboButton::ChildVisibilityChanged(views::View* child) {
   UpdateStyles();
 }
@@ -210,7 +253,11 @@ std::unique_ptr<TabStripFlatEdgeButton>
 TabStripComboButton::CreateFlatEdgeButtonFor(ui::ElementIdentifier element_id) {
   auto button = std::make_unique<TabStripFlatEdgeButton>();
   button->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_CENTER);
-  button->SetShouldShowLabel(context_ == Context::kVerticalTabStrip);
+  // 风桥 竖向标签栏顶部的格子 / 搜索只放图标、不带字。上游在
+  // 收起态把按钮拉宽后塞进标签文字，窄栏里只剩「…」，违反「字不许被裁切」；
+  // 设计稿（展开、收起、悬停展开）里这两个按钮都是 32 见方的图标按钮。
+  // 名称照旧由动作项给（提示气泡、读屏不受影响）。
+  button->SetShouldShowLabel(false);
   button->SetExpansionOrientation(orientation_);
   button->set_context_menu_controller(this);
   button->SetProperty(views::kElementIdentifierKey, element_id);
@@ -234,7 +281,10 @@ TabStripComboButton::CreateFlatEdgeButtonFor(actions::ActionId action_id,
 }
 
 void TabStripComboButton::UpdateStyles() {
-  const bool both_visible = start_button_ && start_button_->GetVisible() &&
+  // 风桥 竖向标签栏里格子、搜索是两个独立的 32 见方 tile（设计稿
+  // 36:26925），不拼成一个平边组合，四角都按圆角画。
+  const bool both_visible = context_ != Context::kVerticalTabStrip &&
+                            start_button_ && start_button_->GetVisible() &&
                             end_button_ && end_button_->GetVisible();
   const bool is_vertical = orientation_ == views::LayoutOrientation::kVertical;
 
