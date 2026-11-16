diff --git a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
index 338178e75c..81e1213775 100644
--- a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
+++ b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
@@ -16,7 +16,12 @@
 #include "base/metrics/user_metrics.h"
 #include "base/metrics/user_metrics_action.h"
 #include "base/numerics/safe_conversions.h"
+#include "base/strings/string_util.h"
+#include "base/strings/utf_string_conversions.h"
 #include "base/time/time.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/ui/actions/chrome_action_id.h"
 #include "chrome/browser/ui/animation/browser_animation_controller.h"
 #include "chrome/browser/ui/animation/browser_animation_types.h"
@@ -54,6 +59,7 @@
 #include "chrome/browser/ui/views/tabs/organizer/organizer_panel_view.h"
 #include "chrome/browser/ui/views/tabs/shared/drop_arrow.h"
 #include "chrome/browser/ui/views/tabs/vertical/top_container_button.h"
+#include "chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_focus_swipe_controller.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.h"
@@ -190,6 +196,23 @@ VerticalTabStripRegionView::VerticalTabStripRegionView(
               LayoutConstant::kVerticalTabStripCollapsedVerticalPadding),
           region_horizontal_padding, 0, region_horizontal_padding));
 
+  // 风桥 N7：左下「已连接 N 个 AI」卡片，放在新标签按钮下面、gemini 按钮上面。
+  ai_card_view_ =
+      content_area_view_->AddChildViewAt(
+          std::make_unique<WindriseAiCardView>(browser_view->browser()),
+          *content_area_view_->GetIndexOf(bottom_button_container_) + 1);
+  ai_card_view_->SetProperty(
+      views::kFlexBehaviorKey,
+      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
+                               views::MaximumFlexSizeRule::kPreferred));
+  ai_card_view_->SetProperty(
+      views::kMarginsKey,
+      gfx::Insets::TLBR(0, region_horizontal_padding, 0,
+                        region_horizontal_padding));
+  // 收起态「标志 + 数字」按收起到位时的卡片宽居中（和收起态标签同宽）。
+  ai_card_view_->SetCollapsedContentWidth(
+      tabs::kVerticalTabStripCollapsedWidth - 2 * region_horizontal_padding);
+
   gemini_button_ =
       content_area_view_->AddChildView(std::make_unique<views::View>());
 
@@ -231,9 +254,29 @@ VerticalTabStripRegionView::VerticalTabStripRegionView(
   }
 
   SetNotifyEnterExitOnChild(true);
+  // 158 用 top_button_separator_ 的 SetInactiveColorId() 处理非激活态
+  // 颜色（见本文件构造函数与 ours 中的写法），不再需要 UpdateColors()。
+  if (tab_strip_model()) {
+    tab_strip_model()->AddObserver(this);
+    UpdateAIWorkspaceExpansion(tab_strip_model()->GetActiveTab());
+  }
+  // N6：扩展推来 AI 段（哪些标签是 AI 的）后，重新算一次「保持展开」。
+  // 析构时 BrowserView 的成员可能已经先拆掉，所以把窗口指针存下来供 Detach 用。
+  windrise_ai_window_ = browser_view->browser();
+  if (browseros::windrise::AiWindowState* ai_state =
+          browseros::windrise::AiWindowState::Attach(windrise_ai_window_)) {
+    ai_state_subscription_ = ai_state->AddObserver(base::BindRepeating(
+        &VerticalTabStripRegionView::UpdateAIWorkspaceExpansionFromState,
+        base::Unretained(this)));
+  }
 }
 
 VerticalTabStripRegionView::~VerticalTabStripRegionView() {
+  if (tab_strip_model()) {
+    tab_strip_model()->RemoveObserver(this);
+  }
+  ai_state_subscription_ = {};
+  browseros::windrise::AiWindowState::Detach(windrise_ai_window_);
   for (const auto& lock : hover_locks_) {
     lock->ClearRegionViewOnDestruction();
   }
@@ -934,6 +977,9 @@ void VerticalTabStripRegionView::OnCollapseStateChanged(
   if (tab_strip_view()) {
     tab_strip_view()->SetCollapsedState(collapsed);
   }
+  if (ai_card_view_) {
+    ai_card_view_->SetCollapsed(collapsed);
+  }
 
   if ((state == tabs::VerticalTabStripCollapseState::kCollapsing) ||
       (state == tabs::VerticalTabStripCollapseState::kExpanded &&
@@ -1231,6 +1277,7 @@ void VerticalTabStripRegionView::OnOmniboxPopupVisibilityChanged(bool is_open) {
 void VerticalTabStripRegionView::OnActiveTabChanged(
     const tabs::TabInterface* active_tab) {
   BaseTabStripRegionView::OnActiveTabChanged(active_tab);
+  UpdateAIWorkspaceExpansion(active_tab);
   omnibox_tab_helper_observation_.Reset();
 
   if (active_tab) {
@@ -1242,5 +1289,61 @@ void VerticalTabStripRegionView::OnActiveTabChanged(
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
+  ai_workspace_group_subscription_ = base::CallbackListSubscription();
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
+            (!prefix.empty() &&
+             base::StartsWith(group->visual_data()->title(),
+                              base::UTF8ToUTF16(prefix),
+                              base::CompareCase::SENSITIVE)) ||
+            browseros::windrise::AiGroups::Contains(*group_id);
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
+void VerticalTabStripRegionView::UpdateAIWorkspaceExpansionFromState() {
+  UpdateAIWorkspaceExpansion(tab_strip_model()
+                                 ? tab_strip_model()->GetActiveTab()
+                                 : nullptr);
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
