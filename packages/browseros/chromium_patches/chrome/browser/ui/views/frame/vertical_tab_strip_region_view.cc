diff --git a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
index b7cdb8f..aea6612 100644
--- a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
+++ b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
@@ -14,7 +14,10 @@
 #include "base/metrics/histogram_functions.h"
 #include "base/metrics/user_metrics.h"
 #include "base/metrics/user_metrics_action.h"
+#include "base/strings/string_util.h"
+#include "base/strings/utf_string_conversions.h"
 #include "base/time/time.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
 #include "chrome/browser/ui/actions/chrome_action_id.h"
 #include "chrome/browser/ui/animation/browser_animation_controller.h"
 #include "chrome/browser/ui/animation/browser_animation_types.h"
@@ -196,9 +199,17 @@ VerticalTabStripRegionView::VerticalTabStripRegionView(
 
   SetNotifyEnterExitOnChild(true);
   UpdateColors();
+
+  if (tab_strip_model()) {
+    tab_strip_model()->AddObserver(this);
+    UpdateAIWorkspaceExpansion(tab_strip_model()->GetActiveTab());
+  }
 }
 
 VerticalTabStripRegionView::~VerticalTabStripRegionView() {
+  if (tab_strip_model()) {
+    tab_strip_model()->RemoveObserver(this);
+  }
   for (const auto& lock : hover_locks_) {
     lock->ClearRegionViewOnDestruction();
   }
@@ -1117,6 +1128,7 @@ void VerticalTabStripRegionView::OnOmniboxPopupVisibilityChanged(bool is_open) {
 void VerticalTabStripRegionView::OnActiveTabChanged(
     const tabs::TabInterface* active_tab) {
   BaseTabStripRegionView::OnActiveTabChanged(active_tab);
+  UpdateAIWorkspaceExpansion(active_tab);
   omnibox_tab_helper_observation_.Reset();
 
   if (active_tab) {
@@ -1128,6 +1140,55 @@ void VerticalTabStripRegionView::OnActiveTabChanged(
   }
 }
 
+void VerticalTabStripRegionView::OnTabGroupChanged(
+    const TabGroupChange&) {
+  UpdateAIWorkspaceExpansion(tab_strip_model()
+                                 ? tab_strip_model()->GetActiveTab()
+                                 : nullptr);
+}
+
+void VerticalTabStripRegionView::UpdateAIWorkspaceExpansion(
+    const tabs::TabInterface* active_tab) {
+  ai_workspace_group_subscription_.reset();
+
+  bool keep_expanded = false;
+  if (active_tab && tab_strip_model() && tab_strip_model()->group_model()) {
+    const std::optional<tab_groups::TabGroupId> group_id =
+        active_tab->GetGroup();
+    if (group_id) {
+      TabGroup* const group =
+          tab_strip_model()->group_model()->GetTabGroup(*group_id);
+      if (group) {
+        const std::string& prefix = browseros::GetAIWorkspaceGroupPrefix(
+            browser_view()->browser()->GetProfile()->GetPrefs());
+        keep_expanded =
+            !prefix.empty() &&
+            base::StartsWith(group->visual_data()->title(),
+                             base::UTF8ToUTF16(prefix),
+                             base::CompareCase::SENSITIVE);
+        ai_workspace_group_subscription_ =
+            group->RegisterOnVisualDataChanged(base::BindRepeating(
+                &VerticalTabStripRegionView::OnAIWorkspaceGroupVisualDataChanged,
+                base::Unretained(this)));
+      }
+    }
+  }
+
+  if (keep_expanded && !ai_workspace_lock_) {
+    ai_workspace_lock_ =
+        GetExpandOnHoverLock(ExpandOnHoverLockType::kKeepExpanded);
+  } else if (!keep_expanded) {
+    ai_workspace_lock_.reset();
+  }
+  UpdateExpandOnHoverState();
+}
+
+void VerticalTabStripRegionView::OnAIWorkspaceGroupVisualDataChanged() {
+  UpdateAIWorkspaceExpansion(tab_strip_model()
+                                 ? tab_strip_model()->GetActiveTab()
+                                 : nullptr);
+}
+
 BEGIN_METADATA(VerticalTabStripRegionView)
 END_METADATA
