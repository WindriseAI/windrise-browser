diff --git a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.cc b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.cc
index 2358db3c42..f71770885e 100644
--- a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.cc
+++ b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.cc
@@ -444,6 +444,18 @@ void TabHoverCardController::CreateHoverCard(
   params.show_domain = !IsBrowserForSystemWebApp(browser_window_interface_);
   params.show_memory_usage = hover_card_memory_usage_enabled_;
   params.show_image_preview = hover_card_image_previews_enabled_;
+  // 风桥 theme-v2 §4：只有悬停收起侧栏（70 宽）里的标签时用收起态样式。按
+  // 锚点自己的宽度判断，所以悬停展开（侧栏临时撑宽、显示标题）时仍是上游样式。
+  if (const auto* vertical_state_controller =
+          tabs::VerticalTabStripStateController::From(
+              browser_window_interface_.get());
+      vertical_state_controller &&
+      vertical_state_controller->ShouldDisplayVerticalTabs() &&
+      anchor_target->GetView() &&
+      anchor_target->GetView()->width() <=
+          tabs::kVerticalTabStripCollapsedWidth) {
+    params.windrise_rail_style = true;
+  }
 
   hover_card_ = new TabHoverCardBubbleView(anchor_target, params);
   hover_card_observation_.Observe(hover_card_.get());
