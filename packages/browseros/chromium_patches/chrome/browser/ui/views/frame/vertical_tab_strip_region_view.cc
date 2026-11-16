diff --git a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
index 338178e75c..9843a6828a 100644
--- a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
+++ b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
@@ -17,6 +17,8 @@
 #include "base/metrics/user_metrics_action.h"
 #include "base/numerics/safe_conversions.h"
 #include "base/time/time.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/ui/actions/chrome_action_id.h"
 #include "chrome/browser/ui/animation/browser_animation_controller.h"
 #include "chrome/browser/ui/animation/browser_animation_types.h"
@@ -54,6 +56,9 @@
 #include "chrome/browser/ui/views/tabs/organizer/organizer_panel_view.h"
 #include "chrome/browser/ui/views/tabs/shared/drop_arrow.h"
 #include "chrome/browser/ui/views/tabs/vertical/top_container_button.h"
+#include "chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h"
+#include "chrome/browser/ui/views/frame/windrise_ai_section_view.h"
+#include "chrome/browser/ui/views/tabs/common/unpinned_tab_container_view.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_focus_swipe_controller.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.h"
@@ -190,6 +195,45 @@ VerticalTabStripRegionView::VerticalTabStripRegionView(
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
+  // 风桥 N6：侧栏下段（AI 的标签）。放在「+」和左下卡片之间：「+」是唯一一个
+  // 可以无限伸长的子视图，两段之间的空白都归它，所以这一段贴着卡片、从下往上
+  // 排；两段放不下时段高由它自己按用户拖的比例算，上段（kScaleToMinimum）
+  // 拿剩下的。和标签同宽（左右留白同标签列），和卡片之间留一个标签列的上下
+  // 留白。
+  ai_section_view_ = content_area_view_->AddChildViewAt(
+      std::make_unique<WindriseAiSectionView>(
+          browser_view->browser(), browser_view->browser()->GetTabStripModel(),
+          browser_view->browser()->GetProfile()->GetPrefs()),
+      *content_area_view_->GetIndexOf(ai_card_view_));
+  ai_section_view_->SetProperty(
+      views::kFlexBehaviorKey,
+      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
+                               views::MaximumFlexSizeRule::kPreferred));
+  ai_section_view_->SetProperty(
+      views::kMarginsKey,
+      gfx::Insets::TLBR(
+          0, region_horizontal_padding,
+          GetLayoutConstant(
+              LayoutConstant::kVerticalTabStripCollapsedVerticalPadding),
+          region_horizontal_padding));
+
   gemini_button_ =
       content_area_view_->AddChildView(std::make_unique<views::View>());
 
@@ -231,9 +275,29 @@ VerticalTabStripRegionView::VerticalTabStripRegionView(
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
@@ -409,6 +473,40 @@ void VerticalTabStripRegionView::AddedToWidget() {
   if (GetFocusManager()) {
     GetFocusManager()->AddFocusChangeListener(&focus_listener_);
   }
+  // 风桥 L11：打开时窗口就窄，也要收起。
+  if (!GetWidget()->IsMinimized()) {
+    UpdateWindriseNarrowWindow(GetWidget()->GetWindowBoundsInScreen().width());
+  }
+}
+
+void VerticalTabStripRegionView::OnWidgetBoundsChanged(
+    views::Widget* widget,
+    const gfx::Rect& new_bounds) {
+  // 最小化时报上来的宽不算（不然还原时会先收起再展开一遍）。
+  if (widget->IsMinimized()) {
+    return;
+  }
+  UpdateWindriseNarrowWindow(new_bounds.width());
+}
+
+void VerticalTabStripRegionView::UpdateWindriseNarrowWindow(int window_width) {
+  // 风桥 L11（丙12）：窗口宽 < 900 时侧栏自动收起，但不写用户偏好；宽回来
+  // 以后恢复用户自己的选择（状态控制器里做）。进出各留一点回差（< 900 进，
+  // >= 920 出），拖窗口边在 900 附近来回时侧栏不来回开合。
+  constexpr int kNarrowEnterWidth = 900;
+  constexpr int kNarrowExitWidth = 920;
+  // 标签条还没接上时先不判断（收起请求会落空），接上时 AddTabStripView 再调。
+  if (window_width <= 0 || !tab_strip_view()) {
+    return;
+  }
+  const bool narrow = windrise_narrow_window_
+                          ? window_width < kNarrowExitWidth
+                          : window_width < kNarrowEnterWidth;
+  if (narrow == windrise_narrow_window_) {
+    return;
+  }
+  windrise_narrow_window_ = narrow;
+  state_controller_->SetWindriseNarrowWindow(narrow);
 }
 
 void VerticalTabStripRegionView::RemovedFromWidget() {
@@ -885,7 +983,13 @@ void VerticalTabStripRegionView::AddTabStripView(
   std::optional<size_t> separator_index =
       content_parent->GetIndexOf(top_button_separator_);
   CHECK(separator_index.has_value());
-  content_parent->AddChildViewAt(std::move(view), separator_index.value() + 1);
+  views::View* const tab_strip =
+      content_parent->AddChildViewAt(std::move(view),
+                                     separator_index.value() + 1);
+  // 风桥 N6：下段要看上段（人的标签）要多高，才知道两段碰没碰到。
+  if (ai_section_view_) {
+    ai_section_view_->SetHumanSection(tab_strip, bottom_button_container_);
+  }
 
   // Pre-set the animation values to the appropriate state.
   auto* const animation_controller =
@@ -907,6 +1011,12 @@ void VerticalTabStripRegionView::AddTabStripView(
               base::Unretained(this)));
 
   OnCollapseStateChanged(state_controller_->GetCollapseState());
+
+  // 风桥 L11：标签条接上、动画订阅好以后，收起才有动画可走（RequestCollapse
+  // 在没有标签条时直接返回），所以窄窗判断放在这里再做一次。
+  if (GetWidget() && !GetWidget()->IsMinimized()) {
+    UpdateWindriseNarrowWindow(GetWidget()->GetWindowBoundsInScreen().width());
+  }
 }
 
 std::unique_ptr<views::View> VerticalTabStripRegionView::RemoveTabStripView(
@@ -916,6 +1026,9 @@ std::unique_ptr<views::View> VerticalTabStripRegionView::RemoveTabStripView(
   omnibox_tab_helper_observation_.Reset();
   ResetExpandOnHoverTimers();
   is_expanded_on_hover_ = false;
+  if (ai_section_view_) {
+    ai_section_view_->SetHumanSection(nullptr, bottom_button_container_);
+  }
   return BaseTabStripRegionView::RemoveTabStripView(view);
 }
 
@@ -934,6 +1047,9 @@ void VerticalTabStripRegionView::OnCollapseStateChanged(
   if (tab_strip_view()) {
     tab_strip_view()->SetCollapsedState(collapsed);
   }
+  if (ai_card_view_) {
+    ai_card_view_->SetCollapsed(collapsed);
+  }
 
   if ((state == tabs::VerticalTabStripCollapseState::kCollapsing) ||
       (state == tabs::VerticalTabStripCollapseState::kExpanded &&
@@ -1231,6 +1347,7 @@ void VerticalTabStripRegionView::OnOmniboxPopupVisibilityChanged(bool is_open) {
 void VerticalTabStripRegionView::OnActiveTabChanged(
     const tabs::TabInterface* active_tab) {
   BaseTabStripRegionView::OnActiveTabChanged(active_tab);
+  UpdateAIWorkspaceExpansion(active_tab);
   omnibox_tab_helper_observation_.Reset();
 
   if (active_tab) {
@@ -1242,5 +1359,62 @@ void VerticalTabStripRegionView::OnActiveTabChanged(
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
+        // 风桥 N6：AI 的组只认 browserOS.setAiTabs 建的（AiGroups），不再按
+        // 组标题以「AI 」开头猜。
+        keep_expanded = browseros::windrise::AiGroups::Contains(*group_id);
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
+  // AI 组变了：上段要重新排一次，把新的 AI 组藏起来、清掉的组放回来。
+  if (tab_strip_view()) {
+    if (views::View* unpinned = tab_strip_view()->GetUnpinnedTabsContainer()) {
+      unpinned->InvalidateLayout();
+    }
+  }
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
