diff --git a/chrome/browser/ui/views/toolbar/toolbar_view.cc b/chrome/browser/ui/views/toolbar/toolbar_view.cc
index 7f404c6864..9fde511292 100644
--- a/chrome/browser/ui/views/toolbar/toolbar_view.cc
+++ b/chrome/browser/ui/views/toolbar/toolbar_view.cc
@@ -23,6 +23,7 @@
 #include "build/build_config.h"
 #include "chrome/app/chrome_command_ids.h"
 #include "chrome/browser/actor/ui/actor_ui_metrics.h"
+#include "chrome/browser/browseros/customization/customization_pref_names.h"
 #include "chrome/browser/command_updater.h"
 #include "chrome/browser/glic/browser_ui/activity_list_bubble/glic_activity_list_bubble.h"
 #include "chrome/browser/glic/browser_ui/activity_list_bubble/glic_activity_list_bubble_controller.h"
@@ -118,6 +119,7 @@
 #include "chrome/browser/ui/views/toolbar/toolbar_icon_container_view.h"
 #include "chrome/browser/ui/views/toolbar/webui_back_forward_control.h"
 #include "chrome/browser/ui/views/toolbar/webui_toolbar_web_view.h"
+#include "chrome/browser/ui/views/toolbar/windrise_grid_button.h"
 #include "chrome/browser/ui/views/zoom/zoom_view_controller.h"
 #include "chrome/browser/ui/waap/initial_webui_window_metrics_manager.h"
 #include "chrome/browser/ui/web_applications/app_browser_controller.h"
@@ -416,7 +418,14 @@ void ToolbarView::Init() {
   // profile (only regular and incognito profiles host extensions).
   if (!browser_->GetProfile()->IsGuestSession() &&
       !features::IsWebUIExtensionsContainerEnabled()) {
-    extensions_container = std::make_unique<ExtensionsToolbarDesktop>(browser_);
+    // BrowserOS: with browseros.show_extensions_toolbar off, e.g. forced by
+    // --disable-browseros-features=ExtensionsToolbar, use the auto-hide mode of
+    // web-app windows. The area then appears only while an extension popup or
+    // the extensions menu needs it, so popups keep their anchor.
+    extensions_container = std::make_unique<ExtensionsToolbarDesktop>(
+        browser_, prefs->GetBoolean(browseros::prefs::kShowExtensionsToolbar)
+                      ? ExtensionsToolbarDesktop::DisplayMode::kNormal
+                      : ExtensionsToolbarDesktop::DisplayMode::kAutoHide);
 
     toolbar_divider = std::make_unique<ToolbarDivider>();
   }
@@ -507,6 +516,11 @@ void ToolbarView::Init() {
     }
   }
 
+  // 风桥「格子」按钮（方案 N8）：地址栏（和 glic 按钮）之后、扩展按钮之前。
+  // 尺寸与配色都从偏好取，见 WindriseGridButton。
+  windrise_grid_button_ =
+      AddChildView(std::make_unique<WindriseGridButton>(browser_));
+
   if (extensions_container) {
     extensions_container_ = AddChildView(std::move(extensions_container));
     extensions_toolbar_coordinator_ =
@@ -607,6 +621,12 @@ void ToolbarView::Init() {
     avatar_ = AddChildView(std::make_unique<AvatarToolbarButton>(browser_));
     bool show_avatar_toolbar_button =
         AvatarToolbarButtonInterface::CanShowForProfile(browser_->GetProfile());
+    // BrowserOS: browseros.show_avatar_button can hide it, e.g. forced off by
+    // --disable-browseros-features=AvatarButton. Nothing changes the avatar's
+    // visibility after this, so reading the pref once per window is enough.
+    if (!prefs->GetBoolean(browseros::prefs::kShowAvatarButton)) {
+      show_avatar_toolbar_button = false;
+    }
     avatar_->SetVisible(show_avatar_toolbar_button);
   }
 
@@ -1577,6 +1597,12 @@ void ToolbarView::InitLayout() {
           : kOrderOffset + 1;
   constexpr int kToolbarActionsFlexOrder = kOrderOffset + 2;
   constexpr int kExtensionsFlexOrder = kOrderOffset + 3;
+  // 风桥「格子」按钮排在扩展容器前面拿位置（order 越小越先拿到、越晚让位）：
+  // 用户 R231「固定四五个扩展……都能固定上」同时不许挤掉我们自己的按钮。窗口
+  // 不够宽时先把放不下的扩展收进拼图菜单，格子按钮留着；窄窗（< 900）时格子
+  // 按钮本身已收成 32 见方的图标（WindriseGridButton::UpdateCompact）。和
+  // 「固定的工具栏动作」同一档（kToolbarActionsFlexOrder）。
+  constexpr int kGridButtonFlexOrder = kToolbarActionsFlexOrder;
 
   const views::FlexSpecification location_bar_flex_rule =
       views::FlexSpecification(views::LayoutOrientation::kHorizontal,
@@ -1610,6 +1636,17 @@ void ToolbarView::InitLayout() {
                                        extensions_flex_rule);
   }
 
+  if (windrise_grid_button_) {
+    // 极窄（连地址栏都到了最小）时才整颗让出；在那之前扩展先收进菜单。
+    windrise_grid_button_->SetProperty(
+        views::kFlexBehaviorKey,
+        views::FlexSpecification(
+            views::LayoutOrientation::kHorizontal,
+            views::MinimumFlexSizeRule::kPreferredSnapToZero,
+            views::MaximumFlexSizeRule::kPreferred)
+            .WithOrder(kGridButtonFlexOrder));
+  }
+
   if (pinned_toolbar_actions_container_) {
     pinned_toolbar_actions_container_->SetProperty(
         views::kFlexBehaviorKey,
