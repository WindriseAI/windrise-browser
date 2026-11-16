diff --git a/ui/views/bubble/bubble_dialog_delegate_view.h b/ui/views/bubble/bubble_dialog_delegate_view.h
index c63a5501d9..4daf9f8c75 100644
--- a/ui/views/bubble/bubble_dialog_delegate_view.h
+++ b/ui/views/bubble/bubble_dialog_delegate_view.h
@@ -64,6 +64,7 @@ class ExtensionInstallDialogView;
 class ExtensionPopup;
 class ExtensionsMenuView;
 class FlyingIndicator;
+class WindriseAiPanelView;
 class GlobalErrorBubbleView;
 class HomePageUndoBubble;
 class MediaDialogView;
@@ -510,6 +511,17 @@ class VIEWS_EXPORT BubbleDialogDelegate : public DialogDelegate {
   bool accept_events() const { return accept_events_; }
   void set_accept_events(bool accept_events) { accept_events_ = accept_events; }
 
+  // Windrise : every bubble gets the shared open / close
+  // animation (open 240ms: opacity 0->1 + scale 0.96->1 from the anchor side;
+  // close 180ms: opacity only; curve gfx::Tween::WINDRISE_EASE_OUT). Bubbles
+  // that run their own fade (the tab hover card) keep upstream's default.
+  bool windrise_use_default_visibility_animation() const {
+    return windrise_use_default_visibility_animation_;
+  }
+  void set_windrise_use_default_visibility_animation(bool use_default) {
+    windrise_use_default_visibility_animation_ = use_default;
+  }
+
   // Whether the bubble accepts drag and drop.
   bool accept_drag_drop() const { return accept_drag_drop_; }
   void set_accept_drag_drop(bool accept_drag_drop) {
@@ -774,6 +786,7 @@ class VIEWS_EXPORT BubbleDialogDelegate : public DialogDelegate {
   mutable std::optional<gfx::Rect> anchor_rect_;
 
   bool accept_events_ = true;
+  bool windrise_use_default_visibility_animation_ = false;
   bool accept_drag_drop_ = true;
   gfx::NativeView parent_window_ = gfx::NativeView();
 
@@ -921,6 +934,8 @@ class VIEWS_EXPORT BubbleDialogDelegateView : public View,
   friend class ::ProfileMenuViewBase;
   friend class ::RemoveSuggestionBubbleDialogDelegateView;
   friend class ::StoragePressureBubbleView;
+  // 风桥 左下「已连接 N 个 AI」面板，走 CreatePassKey() 构造。
+  friend class ::WindriseAiPanelView;
   friend class ::TabGroupEditorBubbleView;
   friend class ::TabHoverCardBubbleView;
   friend class ::TestBubbleView;
