diff --git a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
index 5c8d5a6..47d4c02 100644
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
@@ -36,6 +37,7 @@
 
 class BrowserView;
 class VerticalTabStripTopContainer;
+class WindriseAiCardView;
 class VerticalTabStripBottomContainer;
 class VerticalTabStripFocusSwipeController;
 class ShadowFrameView;
@@ -58,6 +60,7 @@ class VerticalTabStripRegionView final
       public OmniboxTabHelper::Observer,
       public tabs::VerticalTabStripStateController::Delegate,
       public OrganizerPanelHost,
+      public TabStripModelObserver,
       public views::LayoutDelegate {
   METADATA_HEADER(VerticalTabStripRegionView, BaseTabStripRegionView)
 
@@ -211,6 +214,12 @@ class VerticalTabStripRegionView final
   void RegisterExpandOnHoverLock(VerticalTabStripExpandOnHoverLock* lock);
   void UnregisterExpandOnHoverLock(VerticalTabStripExpandOnHoverLock* lock);
 
+  // Keeps the vertical tab strip expanded while the active tab belongs to an
+  // AI workspace tab group.
+  void UpdateAIWorkspaceExpansion(const tabs::TabInterface* active_tab);
+  void UpdateAIWorkspaceExpansionFromState();
+  void OnAIWorkspaceGroupVisualDataChanged();
+
   // OmniboxTabHelper::Observer:
   void OnOmniboxInputStateChanged() override {}
   void OnOmniboxInputInProgress(bool in_progress) override {}
@@ -225,6 +234,11 @@ class VerticalTabStripRegionView final
   double organizer_panel_show_percent_ = 0.0;
 
   raw_ptr<views::View> content_area_view_ = nullptr;
+
+  // TabStripModelObserver:
+  void OnTabGroupChanged(const TabGroupChange& change) override;
+  raw_ptr<WindriseAiCardView> ai_card_view_ = nullptr;
+  base::CallbackListSubscription ai_state_subscription_;
   raw_ptr<VerticalTabStripTopContainer> top_button_container_ = nullptr;
   raw_ptr<views::Separator> top_button_separator_ = nullptr;
   raw_ptr<VerticalTabStripBottomContainer> bottom_button_container_ = nullptr;
@@ -274,6 +288,8 @@ class VerticalTabStripRegionView final
   int keep_expanded_lock_count_ = 0;
   std::unique_ptr<ExpandOnHoverLock> omnibox_open_lock_;
   std::unique_ptr<ExpandOnHoverLock> link_drag_lock_;
+  std::unique_ptr<ExpandOnHoverLock> ai_workspace_lock_;
+  base::CallbackListSubscription ai_workspace_group_subscription_;
   base::flat_set<raw_ptr<VerticalTabStripExpandOnHoverLock>> hover_locks_;
 
   std::unique_ptr<TabHoverCardController::ScopedHideHoverCardLock>
