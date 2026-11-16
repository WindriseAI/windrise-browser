diff --git a/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc b/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc
index c34734562b..5d98cc05db 100644
--- a/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc
+++ b/chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc
@@ -4,10 +4,15 @@
 
 #include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.h"
 
+#include <algorithm>
+#include <optional>
+
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/ui/actions/chrome_action_id.h"
 #include "chrome/browser/ui/browser_commands.h"
 #include "chrome/browser/ui/browser_element_identifiers.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
 #include "chrome/browser/ui/layout_constants.h"
 #include "chrome/browser/ui/tabs/organizer/organizer_panel_utils.h"
 #include "chrome/browser/ui/tabs/saved_tab_groups/saved_tab_group_utils.h"
@@ -27,6 +32,7 @@
 #include "ui/base/l10n/l10n_util.h"
 #include "ui/base/metadata/metadata_impl_macros.h"
 #include "ui/compositor/layer.h"
+#include "ui/gfx/animation/tween.h"
 #include "ui/views/actions/action_view_controller.h"
 #include "ui/views/controls/button/label_button.h"
 #include "ui/views/controls/menu/menu_runner.h"
@@ -39,6 +45,57 @@ DEFINE_CLASS_ELEMENT_IDENTIFIER_VALUE(
     VerticalTabStripTopContainer,
     kToggleVerticalTabsExpandOnHoverElementId);
 
+namespace {
+
+// 风桥 悬停展开就是展开。格子、搜索从收起时的竖排
+// 位置走到展开帧 36:26925 的右上横排，和点住展开时一样；只有收起时才竖排。
+// 收起栏内容宽 = 66 − 2×8 = 50；悬停全宽照 tabbed layout 的算法是
+// 收起宽 + (240 − 收起宽) × 悬停值，内容宽 224。
+int CollapsedContentWidth() {
+  return tabs::kVerticalTabStripCollapsedWidth -
+         2 * GetLayoutConstant(
+                 LayoutConstant::kVerticalTabStripHorizontalPadding);
+}
+
+int ExpandOnHoverContentWidth() {
+  return std::max(tabs::kVerticalTabStripDefaultUncollapsedWidth,
+                  tabs::kVerticalTabStripCollapsedWidth) -
+         2 * GetLayoutConstant(
+                 LayoutConstant::kVerticalTabStripHorizontalPadding);
+}
+
+// 收起态下返回 0（收起）到 1（悬停展开到位）的进度；没收起时返回 nullopt，
+// 照原来的分支排。收起态的栏只会经由悬停宽度变宽：鼠标悬停，或者
+// keep-expanded 锁（AI 工作区分组、菜单等，「悬停时展开」没开也会展开），
+// 所以这里不看「悬停时展开」开没开，只看宽度。进度不另起动画，直接取栏此刻的
+// 宽度：(宽 − 50) / 174，恰好等于 TabStripAnimations::kTabStripHoverWidth
+// 的当前值。这条值展开 240ms、收回 180ms、曲线 WINDRISE_EASE_OUT，中途反向时
+// kStartAtOldValue 从当前值接着走，所以按钮的时长、曲线、打断都和侧栏一致，
+// 不会先清空再重画；减少动态效果时栏直接到位，按钮随栏。
+std::optional<double> GetExpandOnHoverProgress(
+    tabs::VerticalTabStripStateController* state_controller,
+    int content_width) {
+  if (!state_controller || !state_controller->IsCollapsed()) {
+    return std::nullopt;
+  }
+  // 上游在 organizer panel 功能打开、又开了悬停时展开时另有一套横排
+  // （keep_horizontal_layout），那条路不动。
+  if (state_controller->IsExpandOnHoverEnabled() &&
+      organizer_panel::IsOrganizerPanelFeatureEnabled()) {
+    return std::nullopt;
+  }
+  const int collapsed_width = CollapsedContentWidth();
+  const int expanded_width = ExpandOnHoverContentWidth();
+  if (expanded_width <= collapsed_width) {
+    return std::nullopt;
+  }
+  return std::clamp(static_cast<double>(content_width - collapsed_width) /
+                        (expanded_width - collapsed_width),
+                    0.0, 1.0);
+}
+
+}  // namespace
+
 VerticalTabStripTopContainer::VerticalTabStripTopContainer(
     tabs::VerticalTabStripStateController* state_controller,
     actions::ActionItem* root_action_item,
@@ -95,8 +152,39 @@ VerticalTabStripTopContainer::VerticalTabStripTopContainer(
 VerticalTabStripTopContainer::~VerticalTabStripTopContainer() = default;
 
 void VerticalTabStripTopContainer::Layout(PassKey) {
+  // 风桥 悬停展开时朝向和逐个按钮的位置都按这一刻的
+  // 实际宽度算，不用 CalculateProposedLayout 为别的尺寸（首选 / 最小尺寸
+  // 询问）留下的朝向。到位（进度 1）才换成横排，中间由组合按钮逐个插值。
+  // 必须在 LayoutSuperclass 之前交给组合按钮：LayoutSuperclass 给它新外框时
+  // 它会当场排里面的按钮，进度晚一步给，这一帧就是新外框配上一帧的按钮位置。
+  const std::optional<double> hover_progress =
+      collapse_button_ ? GetExpandOnHoverProgress(state_controller_, width())
+                       : std::nullopt;
+  if (hover_progress.has_value()) {
+    combo_button_orientation_ = *hover_progress >= 1.0
+                                    ? views::LayoutOrientation::kHorizontal
+                                    : views::LayoutOrientation::kVertical;
+    combo_button_->SetOrientation(combo_button_orientation_);
+  }
+  combo_button_->SetOrientationTransitionProgress(
+      hover_progress.value_or(0.0));
   LayoutSuperclass<views::View>(this);
   combo_button_->SetOrientation(combo_button_orientation_);
+  // 风桥 格子、搜索和收起按钮一样，各自是一块 #E8EBEE 底、
+  // 圆角 10 的 32 见方 tile（/ 方案:18、设计稿 36:26925 与点头页
+  // ntp-zh-rail）。颜色走 mixer 的 kColorWindriseTileBackground，圆角走偏好；
+  // 每次布局给一次（两个 setter 值没变就直接返回），改偏好即改外观。
+  const float top_button_radius = static_cast<float>(
+      browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTopButtonRadius));
+  for (TabStripFlatEdgeButton* button :
+       {combo_button_->start_button(), combo_button_->end_button()}) {
+    if (!button) {
+      continue;
+    }
+    button->SetBackgroundColorId(kColorWindriseTileBackground);
+    button->SetCornerRadius(top_button_radius);
+  }
   combo_button_->SetClipLayerToVisibleBounds(
       !GetLocalBounds().Contains(combo_button_->bounds()));
 }
@@ -139,8 +227,19 @@ views::ProposedLayout VerticalTabStripTopContainer::CalculateProposedLayout(
       (trailing_control_width >= available_width ||
        available_width < tabs::kVerticalTabStripCollapseSnapWidth ||
        is_collapsed)) {
-    combo_button_orientation_ = views::LayoutOrientation::kVertical;
+    // 风桥 收起 + 悬停展开时的进度（见
+    // GetExpandOnHoverProgress）。0 就是收起态，下面照原样排；大于 0 时组合
+    // 按钮和顶部高度从收起态的位置朝展开态的位置插值，1 时和点住展开一样。
+    const double hover_progress =
+        collapse_button_
+            ? GetExpandOnHoverProgress(state_controller_, available_width)
+                  .value_or(0.0)
+            : 0.0;
+    combo_button_orientation_ = hover_progress >= 1.0
+                                    ? views::LayoutOrientation::kHorizontal
+                                    : views::LayoutOrientation::kVertical;
     int current_y = 0;
+    gfx::Rect collapse_bounds;
 
     if (segmented_control_) {
       layout.child_layouts.emplace_back(segmented_control_.get(),
@@ -167,6 +266,7 @@ views::ProposedLayout VerticalTabStripTopContainer::CalculateProposedLayout(
       layout.child_layouts.emplace_back(collapse_button_.get(),
                                         /*visible=*/true, bounds);
       host_size.SetToMax(gfx::Size(bounds.right(), 0));
+      collapse_bounds = bounds;
 
       current_y += pref_size.height();
     }
@@ -184,22 +284,42 @@ views::ProposedLayout VerticalTabStripTopContainer::CalculateProposedLayout(
 
         const gfx::Size pref_size =
             combo_button_->GetPreferredSizeForOrientation(
-                combo_button_orientation_);
-
-        const int kComboButtonCollapsedPadding = GetLayoutConstant(
-            LayoutConstant::kVerticalTabStripFlatEdgeButtonPadding);
-
-        // When collapsed the combo button should fill the width (excluding
-        // padding), this is especially relevant when in the expand-on-hover
-        // state.
-        int width =
-            is_collapsed
-                ? (host_size.width() - (2 * kComboButtonCollapsedPadding))
-                : pref_size.width();
-        int padding = is_collapsed
-                          ? kComboButtonCollapsedPadding
-                          : std::max(0, (host_size.width() - width) / 2);
+                views::LayoutOrientation::kVertical);
+
+        // 风桥 上游收起时把组合按钮撑满栏宽，按钮比 32 宽就
+        // 塞进标签文字，窄栏里露出「…」。这里一律取首选宽（两个 32 见方竖排），
+        // 在收起栏的标签列里水平居中（点头页 ntp-zh-rail：66 栏里居中）。
+        const int collapsed_content_width = CollapsedContentWidth();
+        const int width = pref_size.width();
+        const int centering_width =
+            is_collapsed ? std::min(host_size.width(), collapsed_content_width)
+                         : host_size.width();
+        const int padding = std::max(0, (centering_width - width) / 2);
         gfx::Rect bounds(padding, current_y, width, pref_size.height());
+
+        // 风桥 悬停展开时，组合按钮从上面的收起位置
+        // 朝展开态位置插值。展开态取横排分支在「收起 + 悬停」下的结果：和收起
+        // 按钮同一行（行中线对齐），右对齐到悬停全宽。右对齐用全宽而不用此刻
+        // 的宽度，x 一路单调往右，按钮右沿始终不超出栏的右沿。四个分量各自
+        // 线性插值，和组合按钮里逐个按钮的插值（SetOrientationTransitionProgress）
+        // 取整一致，里面的按钮不会被裁掉一像素。
+        if (hover_progress > 0.0) {
+          const gfx::Size row_size =
+              combo_button_->GetPreferredSizeForOrientation(
+                  views::LayoutOrientation::kHorizontal);
+          const gfx::Rect expanded_bounds(
+              std::max(0, ExpandOnHoverContentWidth() - row_size.width()),
+              std::max(0, collapse_bounds.CenterPoint().y() -
+                              row_size.height() / 2),
+              row_size.width(), row_size.height());
+          const auto lerp = [hover_progress](int from, int to) {
+            return gfx::Tween::LinearIntValueBetween(hover_progress, from, to);
+          };
+          bounds = gfx::Rect(lerp(bounds.x(), expanded_bounds.x()),
+                             lerp(bounds.y(), expanded_bounds.y()),
+                             lerp(bounds.width(), expanded_bounds.width()),
+                             lerp(bounds.height(), expanded_bounds.height()));
+        }
         layout.child_layouts.emplace_back(combo_button_.get(),
                                           /*visible=*/true, bounds);
         host_size.SetToMax(gfx::Size(bounds.right(), 0));
@@ -208,6 +328,20 @@ views::ProposedLayout VerticalTabStripTopContainer::CalculateProposedLayout(
             pref_size.height() +
             GetLayoutConstant(
                 LayoutConstant::kVerticalTabStripCollapsedVerticalPadding);
+
+        // 顶部高度同样插值到展开态的一行高（横排分支在 Windows 上的
+        // baseline 高度），下面的标签跟着同一条进度上移，悬停展开到位时和
+        // 帧 36:26925 一样，标签紧贴在这一行下面。
+        if (hover_progress > 0.0) {
+          const int baseline_height =
+              GetBaselineMinHeight(/*show_segmented_control=*/false);
+          const int expanded_height = std::max(
+              baseline_height,
+              collapse_bounds.bottom() +
+                  (baseline_height - collapse_bounds.height()) / 2);
+          current_y = gfx::Tween::LinearIntValueBetween(
+              hover_progress, current_y, expanded_height);
+        }
       } else {
         layout.child_layouts.emplace_back(combo_button_.get(),
                                           /*visible=*/true);
