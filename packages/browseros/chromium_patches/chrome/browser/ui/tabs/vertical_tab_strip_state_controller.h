diff --git a/chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h b/chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h
index 4fcb26f9e6..1b837ce927 100644
--- a/chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h
+++ b/chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h
@@ -82,6 +82,11 @@ class VerticalTabStripStateController {
 
   virtual const VerticalTabStripState& GetState() const = 0;
 
+  // 风桥 L11：窗口窄（宽 < 900）时由侧栏自动收起，但不写用户偏好；
+  // 宽回来以后恢复用户自己的选择。窄着的时候用户手动展开 / 收起算用户的新
+  // 选择，照常记下。默认什么都不做（测试替身不用实现）。
+  virtual void SetWindriseNarrowWindow(bool narrow) {}
+
   using ResizingChangeCallback =
       base::RepeatingCallback<void(bool is_resizing)>;
   base::CallbackListSubscription RegisterOnResizingChanged(
