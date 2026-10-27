diff --git a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
index 76f8e99..994c44b 100644
--- a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
+++ b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h
@@ -15,6 +15,7 @@
 #include "chrome/browser/ui/animation/browser_animation_types.h"
 #include "chrome/browser/ui/omnibox/omnibox_tab_helper.h"
 #include "chrome/browser/ui/tabs/tab_data.h"
+#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
 #include "chrome/browser/ui/tabs/vertical_tab_strip_state.h"
 #include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"
 #include "chrome/browser/ui/views/frame/base_tab_strip_region_view.h"
@@ -54,7 +55,8 @@ class VerticalTabStripRegionView final
     : public BaseTabStripRegionView,
       public views::ResizeAreaDelegate,
       public OmniboxTabHelper::Observer,
-      public tabs::VerticalTabStripStateController::Delegate {
+      public tabs::VerticalTabStripStateController::Delegate,
+      public TabStripModelObserver {
   METADATA_HEADER(VerticalTabStripRegionView, BaseTabStripRegionView)
 
  public:
@@ -229,6 +231,11 @@ class VerticalTabStripRegionView final
   void RegisterExpandOnHoverLock(VerticalTabStripExpandOnHoverLock* lock);
   void UnregisterExpandOnHoverLock(VerticalTabStripExpandOnHoverLock* lock);
 
+  // Keeps the vertical tab strip expanded while the active tab belongs to an
+  // AI workspace tab group.
+  void UpdateAIWorkspaceExpansion(const tabs::TabInterface* active_tab);
+  void OnAIWorkspaceGroupVisualDataChanged();
+
   // OmniboxTabHelper::Observer:
   void OnOmniboxInputStateChanged() override {}
   void OnOmniboxInputInProgress(bool in_progress) override {}
@@ -238,6 +245,9 @@ class VerticalTabStripRegionView final
 
   void OnActiveTabChanged(const tabs::TabInterface* active_tab) override;
 
+  // TabStripModelObserver:
+  void OnTabGroupChanged(const TabGroupChange& change) override;
+
   raw_ptr<VerticalTabStripTopContainer> top_button_container_ = nullptr;
   raw_ptr<views::Separator> top_button_separator_ = nullptr;
   raw_ptr<VerticalTabStripBottomContainer> bottom_button_container_ = nullptr;
@@ -288,7 +298,9 @@ class VerticalTabStripRegionView final
   int keep_expanded_lock_count_ = 0;
   std::unique_ptr<ExpandOnHoverLock> omnibox_open_lock_;
   std::unique_ptr<ExpandOnHoverLock> link_drag_lock_;
+  std::unique_ptr<ExpandOnHoverLock> ai_workspace_lock_;
+  base::CallbackListSubscription ai_workspace_group_subscription_;
   base::flat_set<raw_ptr<VerticalTabStripExpandOnHoverLock>> hover_locks_;
 
   std::unique_ptr<TabHoverCardController::ScopedHideHoverCardLock>

