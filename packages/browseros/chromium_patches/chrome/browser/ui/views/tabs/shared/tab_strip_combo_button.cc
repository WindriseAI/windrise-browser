diff --git a/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc b/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc
index 0f3e88328c..81ad6fb1ba 100644
--- a/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc
+++ b/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc
@@ -210,7 +210,11 @@ std::unique_ptr<TabStripFlatEdgeButton>
 TabStripComboButton::CreateFlatEdgeButtonFor(ui::ElementIdentifier element_id) {
   auto button = std::make_unique<TabStripFlatEdgeButton>();
   button->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_CENTER);
-  button->SetShouldShowLabel(context_ == Context::kVerticalTabStrip);
+  // 风桥 P1（59c 判定）：竖向标签栏顶部的格子 / 搜索只放图标、不带字。上游在
+  // 收起态把按钮拉宽后塞进标签文字，窄栏里只剩「…」，违反甲47「字不许被裁切」；
+  // 定稿帧（展开、收起、悬停展开）里这两个按钮都是 32 见方的图标按钮。
+  // 名称照旧由动作项给（提示气泡、读屏不受影响）。
+  button->SetShouldShowLabel(false);
   button->SetExpansionOrientation(orientation_);
   button->set_context_menu_controller(this);
   button->SetProperty(views::kElementIdentifierKey, element_id);
@@ -234,7 +238,10 @@ TabStripComboButton::CreateFlatEdgeButtonFor(actions::ActionId action_id,
 }
 
 void TabStripComboButton::UpdateStyles() {
-  const bool both_visible = start_button_ && start_button_->GetVisible() &&
+  // 风桥 P1：竖向标签栏里格子、搜索是两个独立的 32 见方 tile（乙02、定稿帧
+  // 36:26925），不拼成一个平边组合，四角都按圆角画。
+  const bool both_visible = context_ != Context::kVerticalTabStrip &&
+                            start_button_ && start_button_->GetVisible() &&
                             end_button_ && end_button_->GetVisible();
   const bool is_vertical = orientation_ == views::LayoutOrientation::kVertical;
 
