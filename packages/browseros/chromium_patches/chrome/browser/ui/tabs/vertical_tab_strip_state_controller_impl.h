diff --git a/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.h b/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.h
index 97e7fef48c..4c8033ca0f 100644
--- a/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.h
+++ b/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.h
@@ -69,6 +69,8 @@ class VerticalTabStripStateControllerImpl
 
   const VerticalTabStripState& GetState() const override;
 
+  void SetWindriseNarrowWindow(bool narrow) override;
+
  private:
   class ScopedEnableStateLockImpl;
 
@@ -88,6 +90,10 @@ class VerticalTabStripStateControllerImpl
   // uncollapsed width) for startup when session restore is not available.
   void UpdatePrefService();
 
+  // 写进偏好 / 会话的收起状态：窄窗自动收起期间写用户自己的选择，不写这次
+  // 自动收起。
+  bool GetWindrisePersistedCollapsed() const;
+
   // Update the Collapse Button's Action Item (kActionToggleCollapseVertical)
   // based on the Vertical Tab Strip's Collapse State.
   void UpdateCollapseActionItem();
@@ -131,6 +137,11 @@ class VerticalTabStripStateControllerImpl
   bool is_expand_on_hover_enabled_ = false;
   bool is_resizing_ = false;
 
+  // 风桥 L11：窗口是不是窄着；窄着时用户自己的选择；自动收起发出、还没落地。
+  bool windrise_narrow_window_ = false;
+  bool windrise_user_collapsed_ = false;
+  bool windrise_auto_collapse_pending_ = false;
+
   base::OneShotTimer expand_on_hover_iph_startup_timer_;
   base::OneShotTimer expand_on_hover_iph_collapse_timer_;
 };
