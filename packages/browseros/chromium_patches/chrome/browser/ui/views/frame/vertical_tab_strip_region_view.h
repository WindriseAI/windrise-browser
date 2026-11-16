diff --git a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
index 5c8d5a6d20..2d022fc3cb 100644
--- a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
+++ b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
@@ -35,7 +35,10 @@
 #include "ui/views/layout/delegating_layout_manager.h"
 
 class BrowserView;
+class BrowserWindowInterface;
 class VerticalTabStripTopContainer;
+class WindriseAiCardView;
+class WindriseAiSectionView;
 class VerticalTabStripBottomContainer;
 class VerticalTabStripFocusSwipeController;
 class ShadowFrameView;
@@ -211,6 +214,14 @@ class VerticalTabStripRegionView final
   void RegisterExpandOnHoverLock(VerticalTabStripExpandOnHoverLock* lock);
   void UnregisterExpandOnHoverLock(VerticalTabStripExpandOnHoverLock* lock);
 
+  // AI 段归属改变时刷新上段的隐藏行布局。
+  void OnWindriseAiStateChanged();
+  // 风桥 L11：按窗口宽决定侧栏要不要临时收起。
+  void UpdateWindriseNarrowWindow(int window_width);
+  // views::WidgetObserver（BaseTabStripRegionView 已经在观察本窗口）：
+  void OnWidgetBoundsChanged(views::Widget* widget,
+                             const gfx::Rect& new_bounds) override;
+
   // OmniboxTabHelper::Observer:
   void OnOmniboxInputStateChanged() override {}
   void OnOmniboxInputInProgress(bool in_progress) override {}
@@ -225,6 +236,14 @@ class VerticalTabStripRegionView final
   double organizer_panel_show_percent_ = 0.0;
 
   raw_ptr<views::View> content_area_view_ = nullptr;
+
+  raw_ptr<WindriseAiCardView> ai_card_view_ = nullptr;
+  // 风桥 侧栏下段（AI 的标签），在「+」和左下卡片之间。
+  raw_ptr<WindriseAiSectionView> ai_section_view_ = nullptr;
+  // 风桥 L11：窗口是不是窄着（< 900 进，>= 920 出）。
+  bool windrise_narrow_window_ = false;
+  base::CallbackListSubscription ai_state_subscription_;
+  raw_ptr<BrowserWindowInterface> windrise_ai_window_ = nullptr;
   raw_ptr<VerticalTabStripTopContainer> top_button_container_ = nullptr;
   raw_ptr<views::Separator> top_button_separator_ = nullptr;
   raw_ptr<VerticalTabStripBottomContainer> bottom_button_container_ = nullptr;
