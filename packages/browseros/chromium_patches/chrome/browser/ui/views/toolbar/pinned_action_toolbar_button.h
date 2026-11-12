diff --git a/chrome/browser/ui/views/toolbar/pinned_action_toolbar_button.h b/chrome/browser/ui/views/toolbar/pinned_action_toolbar_button.h
index 8e58f0704c..0d94309975 100644
--- a/chrome/browser/ui/views/toolbar/pinned_action_toolbar_button.h
+++ b/chrome/browser/ui/views/toolbar/pinned_action_toolbar_button.h
@@ -68,6 +68,7 @@ class PinnedActionToolbarButton : public ToolbarButton {
   void SetEngagedIndicatorStyle(ActionItemEngagedIndicatorStyle style);
   void SetActionEngaged(bool action_engaged);
   void UpdateIcon() override;
+  void UpdateLabelVisibility();
   bool ShouldShowEphemerallyInToolbar();
   bool IsIconVisible() { return is_icon_visible_; }
   bool IsPinned() { return pinned_; }
