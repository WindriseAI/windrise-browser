diff --git a/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.h b/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.h
index 61802329b7..f4cc1fbf78 100644
--- a/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.h
+++ b/chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.h
@@ -62,6 +62,11 @@ class TabStripComboButton : public views::View,
 
   void SetOrientation(views::LayoutOrientation orientation);
 
+  // 风桥 竖向标签栏收起 + 悬停展开时，格子、搜索从
+  // 竖排逐个插值到横排。`progress` 0 = 竖排位置，1 = 横排位置；只在 (0, 1)
+  // 之间覆盖 BoxLayout 排出的位置，两端照 SetOrientation 的朝向排。
+  void SetOrientationTransitionProgress(double progress);
+
   TabStripFlatEdgeButton* start_button() { return start_button_; }
   TabStripFlatEdgeButton* end_button() { return end_button_; }
 
@@ -88,6 +93,9 @@ class TabStripComboButton : public views::View,
   gfx::Size GetPreferredSizeForOrientation(
       views::LayoutOrientation orientation);
 
+  // views::View:
+  void Layout(PassKey) override;
+
   void OnMenuClosedForTesting() { OnMenuClosed(); }
   const base::OneShotTimer& hide_tab_search_timer_for_testing() const {
     return hide_tab_search_timer_;
@@ -132,6 +140,7 @@ class TabStripComboButton : public views::View,
   gfx::SlideAnimation end_button_animation_{this};
 
   views::LayoutOrientation orientation_ = views::LayoutOrientation::kHorizontal;
+  double orientation_transition_progress_ = 0.0;
 
   bool show_tab_search_ephemerally_ = false;
 
