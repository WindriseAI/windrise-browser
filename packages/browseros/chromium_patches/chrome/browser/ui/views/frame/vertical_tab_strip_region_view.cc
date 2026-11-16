diff --git a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
index 338178e75c..7668588e29 100644
--- a/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
+++ b/chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc
@@ -17,6 +17,9 @@
 #include "base/metrics/user_metrics_action.h"
 #include "base/numerics/safe_conversions.h"
 #include "base/time/time.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/ui/actions/chrome_action_id.h"
 #include "chrome/browser/ui/animation/browser_animation_controller.h"
 #include "chrome/browser/ui/animation/browser_animation_types.h"
@@ -54,6 +57,9 @@
 #include "chrome/browser/ui/views/tabs/organizer/organizer_panel_view.h"
 #include "chrome/browser/ui/views/tabs/shared/drop_arrow.h"
 #include "chrome/browser/ui/views/tabs/vertical/top_container_button.h"
+#include "chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h"
+#include "chrome/browser/ui/views/frame/windrise_ai_section_view.h"
+#include "chrome/browser/ui/views/tabs/common/unpinned_tab_container_view.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_bottom_container.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_focus_swipe_controller.h"
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.h"
@@ -190,6 +196,60 @@ VerticalTabStripRegionView::VerticalTabStripRegionView(
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
+          browser_view->browser()->GetProfile()->GetPrefs(),
+          base::BindRepeating(
+              [](VerticalTabStripRegionView* region, tabs::TabHandle tab,
+                 const gfx::Point& point,
+                 ui::mojom::MenuSourceType source_type) {
+                if (!tab.Get() || !region->root_node_) {
+                  return;
+                }
+                TabCollectionNode* node =
+                    region->root_node_->GetNodeForHandle(tab);
+                if (node && node->GetController()) {
+                  node->GetController()->ShowTabContextMenu(node, point,
+                                                            source_type);
+                }
+              },
+              base::Unretained(this))),
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
 
@@ -231,9 +291,22 @@ VerticalTabStripRegionView::VerticalTabStripRegionView(
   }
 
   SetNotifyEnterExitOnChild(true);
+  // 158 用 top_button_separator_ 的 SetInactiveColorId() 处理非激活态
+  // 颜色（见本文件构造函数与 ours 中的写法），不再需要 UpdateColors()。
+  // N6：AI 组归属改变时重新排上段；不因为当前标签属于 AI 而撑开侧栏。
+  // 析构时 BrowserView 的成员可能已经先拆掉，所以把窗口指针存下来供 Detach 用。
+  windrise_ai_window_ = browser_view->browser();
+  if (browseros::windrise::AiWindowState* ai_state =
+          browseros::windrise::AiWindowState::Attach(windrise_ai_window_)) {
+    ai_state_subscription_ = ai_state->AddObserver(base::BindRepeating(
+        &VerticalTabStripRegionView::OnWindriseAiStateChanged,
+        base::Unretained(this)));
+  }
 }
 
 VerticalTabStripRegionView::~VerticalTabStripRegionView() {
+  ai_state_subscription_ = {};
+  browseros::windrise::AiWindowState::Detach(windrise_ai_window_);
   for (const auto& lock : hover_locks_) {
     lock->ClearRegionViewOnDestruction();
   }
@@ -409,6 +482,37 @@ void VerticalTabStripRegionView::AddedToWidget() {
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
+  // 以后恢复用户自己的选择（状态控制器里做）。阈值和回差（< 900 进，>= 920
+  // 出）与工具栏格子按钮收成图标共用一份：browseros::windrise::IsNarrowWindow。
+  // 标签条还没接上时先不判断（收起请求会落空），接上时 AddTabStripView 再调。
+  if (window_width <= 0 || !tab_strip_view()) {
+    return;
+  }
+  const bool narrow = browseros::windrise::IsNarrowWindow(
+      window_width, windrise_narrow_window_);
+  if (narrow == windrise_narrow_window_) {
+    return;
+  }
+  windrise_narrow_window_ = narrow;
+  state_controller_->SetWindriseNarrowWindow(narrow);
 }
 
 void VerticalTabStripRegionView::RemovedFromWidget() {
@@ -885,7 +989,13 @@ void VerticalTabStripRegionView::AddTabStripView(
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
@@ -907,6 +1017,12 @@ void VerticalTabStripRegionView::AddTabStripView(
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
@@ -916,6 +1032,9 @@ std::unique_ptr<views::View> VerticalTabStripRegionView::RemoveTabStripView(
   omnibox_tab_helper_observation_.Reset();
   ResetExpandOnHoverTimers();
   is_expanded_on_hover_ = false;
+  if (ai_section_view_) {
+    ai_section_view_->SetHumanSection(nullptr, bottom_button_container_);
+  }
   return BaseTabStripRegionView::RemoveTabStripView(view);
 }
 
@@ -934,6 +1053,9 @@ void VerticalTabStripRegionView::OnCollapseStateChanged(
   if (tab_strip_view()) {
     tab_strip_view()->SetCollapsedState(collapsed);
   }
+  if (ai_card_view_) {
+    ai_card_view_->SetCollapsed(collapsed);
+  }
 
   if ((state == tabs::VerticalTabStripCollapseState::kCollapsing) ||
       (state == tabs::VerticalTabStripCollapseState::kExpanded &&
@@ -1242,5 +1364,14 @@ void VerticalTabStripRegionView::OnActiveTabChanged(
   }
 }
 
+void VerticalTabStripRegionView::OnWindriseAiStateChanged() {
+  // AI 组变了：上段要重新排一次，把新的 AI 组藏起来、清掉的组放回来。
+  if (tab_strip_view()) {
+    if (views::View* unpinned = tab_strip_view()->GetUnpinnedTabsContainer()) {
+      unpinned->InvalidateLayout();
+    }
+  }
+}
+
 BEGIN_METADATA(VerticalTabStripRegionView)
 END_METADATA
