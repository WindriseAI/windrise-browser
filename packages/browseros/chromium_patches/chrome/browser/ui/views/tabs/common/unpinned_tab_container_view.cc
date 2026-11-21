diff --git a/chrome/browser/ui/views/tabs/common/unpinned_tab_container_view.cc b/chrome/browser/ui/views/tabs/common/unpinned_tab_container_view.cc
index 519b66f259..c79a64eb47 100644
--- a/chrome/browser/ui/views/tabs/common/unpinned_tab_container_view.cc
+++ b/chrome/browser/ui/views/tabs/common/unpinned_tab_container_view.cc
@@ -4,6 +4,7 @@
 
 #include "chrome/browser/ui/views/tabs/common/unpinned_tab_container_view.h"
 
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
 #include "chrome/browser/ui/layout_constants.h"
 #include "chrome/browser/ui/tabs/tab_style.h"
 #include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"
@@ -153,6 +154,16 @@ bool UnpinnedTabContainerView::LayoutManager::IsChildVisibleInContainer(
   if (!CanBeVisible(child)) {
     return false;
   }
+  // 风桥 AI 的标签组（windrise.setAiTabs 建的）在竖向标签条里不画在上段，
+  // 由侧边栏下段 WindriseAiSectionView 画成可展开的组合页。
+  if (UnpinnedTabContainerView().orientation_ ==
+      TabStripOrientation::kVertical) {
+    const std::optional<tab_groups::TabGroupId> group_id =
+        GetGroupIdForChild(child);
+    if (group_id && browseros::windrise::AiGroups::Contains(*group_id)) {
+      return false;
+    }
+  }
   auto drag_data =
       UnpinnedTabContainerView().GetVisualDataForDraggedView(*child);
   if (drag_data && drag_data->should_hide) {
