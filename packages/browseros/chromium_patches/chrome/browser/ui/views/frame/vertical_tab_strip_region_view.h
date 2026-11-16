diff --git a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
index 5c8d5a6d20..0a5605d243 100644
--- a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
+++ b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
@@ -16,6 +16,7 @@
 #include "chrome/browser/ui/animation/browser_animation_types.h"
 #include "chrome/browser/ui/omnibox/omnibox_tab_helper.h"
 #include "chrome/browser/ui/tabs/tab_data.h"
+#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
 #include "chrome/browser/ui/tabs/vertical_tab_strip_state.h"
 #include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"
 #include "chrome/browser/ui/views/frame/base_tab_strip_region_view.h"
@@ -35,7 +36,10 @@
 #include "ui/views/layout/delegating_layout_manager.h"
 
 class BrowserView;
+class BrowserWindowInterface;
 class VerticalTabStripTopContainer;
+class WindriseAiCardView;
+class WindriseAiSectionView;
 class VerticalTabStripBottomContainer;
 class VerticalTabStripFocusSwipeController;
 class ShadowFrameView;
@@ -58,6 +62,7 @@ class VerticalTabStripRegionView final
       public OmniboxTabHelper::Observer,
       public tabs::VerticalTabStripStateController::Delegate,
       public OrganizerPanelHost,
+      public TabStripModelObserver,
       public views::LayoutDelegate {
   METADATA_HEADER(VerticalTabStripRegionView, BaseTabStripRegionView)
 
@@ -211,6 +216,17 @@ class VerticalTabStripRegionView final
   void RegisterExpandOnHoverLock(VerticalTabStripExpandOnHoverLock* lock);
   void UnregisterExpandOnHoverLock(VerticalTabStripExpandOnHoverLock* lock);
 
+  // Keeps the vertical tab strip expanded while the active tab belongs to an
+  // AI workspace tab group.
+  void UpdateAIWorkspaceExpansion(const tabs::TabInterface* active_tab);
+  void UpdateAIWorkspaceExpansionFromState();
+  // 风桥 L11：按窗口宽决定侧栏要不要临时收起。
+  void UpdateWindriseNarrowWindow(int window_width);
+  // views::WidgetObserver（BaseTabStripRegionView 已经在观察本窗口）：
+  void OnWidgetBoundsChanged(views::Widget* widget,
+                             const gfx::Rect& new_bounds) override;
+  void OnAIWorkspaceGroupVisualDataChanged();
+
   // OmniboxTabHelper::Observer:
   void OnOmniboxInputStateChanged() override {}
   void OnOmniboxInputInProgress(bool in_progress) override {}
@@ -225,6 +241,16 @@ class VerticalTabStripRegionView final
   double organizer_panel_show_percent_ = 0.0;
 
   raw_ptr<views::View> content_area_view_ = nullptr;
+
+  // TabStripModelObserver:
+  void OnTabGroupChanged(const TabGroupChange& change) override;
+  raw_ptr<WindriseAiCardView> ai_card_view_ = nullptr;
+  // 风桥 N6：侧栏下段（AI 的标签），在「+」和左下卡片之间。
+  raw_ptr<WindriseAiSectionView> ai_section_view_ = nullptr;
+  // 风桥 L11：窗口是不是窄着（< 900 进，>= 920 出）。
+  bool windrise_narrow_window_ = false;
+  base::CallbackListSubscription ai_state_subscription_;
+  raw_ptr<BrowserWindowInterface> windrise_ai_window_ = nullptr;
   raw_ptr<VerticalTabStripTopContainer> top_button_container_ = nullptr;
   raw_ptr<views::Separator> top_button_separator_ = nullptr;
   raw_ptr<VerticalTabStripBottomContainer> bottom_button_container_ = nullptr;
@@ -274,6 +300,8 @@ class VerticalTabStripRegionView final
   int keep_expanded_lock_count_ = 0;
   std::unique_ptr<ExpandOnHoverLock> omnibox_open_lock_;
   std::unique_ptr<ExpandOnHoverLock> link_drag_lock_;
+  std::unique_ptr<ExpandOnHoverLock> ai_workspace_lock_;
+  base::CallbackListSubscription ai_workspace_group_subscription_;
   base::flat_set<raw_ptr<VerticalTabStripExpandOnHoverLock>> hover_locks_;
 
   std::unique_ptr<TabHoverCardController::ScopedHideHoverCardLock>
