diff --git a/chrome/browser/ui/views/tabs/common/tab_view_vertical_layout.cc b/chrome/browser/ui/views/tabs/common/tab_view_vertical_layout.cc
index d2b935e0c8..e774d0679c 100644
--- a/chrome/browser/ui/views/tabs/common/tab_view_vertical_layout.cc
+++ b/chrome/browser/ui/views/tabs/common/tab_view_vertical_layout.cc
@@ -4,6 +4,10 @@
 
 #include "chrome/browser/ui/views/tabs/common/tab_view_vertical_layout.h"
 
+#include <algorithm>
+#include <optional>
+
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/glic/browser_ui/tab_underline_view.h"
 #include "chrome/browser/ui/layout_constants.h"
 #include "chrome/browser/ui/tabs/vertical_tab_strip_state.h"
@@ -14,6 +18,7 @@
 #include "chrome/browser/ui/views/tabs/tab/tab_icon.h"
 #include "chrome/browser/ui/views/tabs/tab/tab_title.h"
 #include "chrome/browser/ui/views/tabs/tab_style_views.h"
+#include "ui/views/controls/label.h"
 #include "ui/views/view_utils.h"
 
 namespace {
@@ -64,6 +69,9 @@ views::ProposedLayout TabViewVerticalLayout::CalculateProposedLayout(
                            !TabView().IsInExpandOnHover(width);
 
   int placed_children = 0;
+  // 风桥：居中（折叠）时唯一放下的那个子视图在 child_layouts 里的下标、设计宽。
+  std::optional<size_t> centered_index;
+  int centered_design_width = kIconDesignWidth;
   for (const auto& child : tab_children_configs_) {
     const bool can_render_child =
         is_centered
@@ -72,6 +80,10 @@ views::ProposedLayout TabViewVerticalLayout::CalculateProposedLayout(
                placed_children < 2);
     const bool is_child_visible = IsChildVisible(child.view, width);
     if (is_child_visible && can_render_child) {
+      if (is_centered) {
+        centered_index = layouts.child_layouts.size();
+        centered_design_width = child.min_width;
+      }
       layouts.child_layouts.emplace_back(
           child.view.get(), is_child_visible,
           GetChildBounds(bounds_remaining, child, is_centered));
@@ -95,6 +107,62 @@ views::ProposedLayout TabViewVerticalLayout::CalculateProposedLayout(
     }
   }
 
+  // 风桥 theme-v2 §4（用户 11-05 / 11-16「收起时露出一个汉字或一个英文大写」）：
+  // 折叠窄条上，图标右边跟一个字，图标 16 + 间距 5 + 字 14，作为一组在整个
+  // 标签宽里居中（不是在扣掉内边距 10/12 后剩下的那块里居中——收起宽 70 时那块
+  // 只有 32，那一组 35 放不下；而且那块的中线比「+」、顶部按钮的中线偏左 1）。
+  // - 字占一个固定 14 宽的格子，汉字、英文大写、数字都在格子里按字形水平
+  //   居中，所以每个标签的图标都在同一条竖线上，不随字宽左右晃。
+  // - 图标那一格就是上游居中分支放下的那一个：平时是图标，有声音 / 录屏等
+  //   提示时是提示图标，当前标签悬停时是关闭按钮——字都跟在它右边。
+  // - 字的竖直位置：让字形中线（基线上方半个大写字母高）对齐图标中线，
+  //   不按行框居中（行框居中字会偏上，见 甲45）。
+  // - 显示与否只看这一次排版：标题不显示（折叠居中）时字就显示，标题一出现
+  //   字就不显示，两者在同一帧换。所以它没有自己的动画，跟着侧栏宽度那条
+  //   动画走（时长、曲线、打断、减少动态效果都和侧栏一样）。
+  // - 固定 / 拆分后放不下这一组（标签宽 < 35）时退回上游：只有图标居中。
+  if (views::Label* const glyph = TabView().rail_glyph_) {
+    const browseros::windrise::LayoutMetrics& metrics =
+        browseros::windrise::GetLayoutMetrics();
+    const int glyph_slot = metrics.Get(
+        browseros::windrise::LayoutMetric::kRailGlyphSize);
+    const int gap = metrics.Get(browseros::windrise::LayoutMetric::kRailGlyphGap);
+    const int group_width = centered_design_width + gap + glyph_slot;
+    const bool show_glyph = centered_index.has_value() &&
+                            !TabView().pinned_ && !glyph->GetText().empty() &&
+                            width >= group_width;
+    if (show_glyph) {
+      views::ChildLayout& lead = layouts.child_layouts[*centered_index];
+      // 一组在整个标签宽里居中（奇数余量多出的 1 给左边）。
+      const int group_x = (width - group_width + 1) / 2;
+      // 图标那一格：和上游非居中时一样，按设计宽 16 摆，实际更宽的（关闭
+      // 按钮）以这一格的中线为中线。
+      lead.bounds.set_x(group_x +
+                        (centered_design_width - lead.bounds.width()) / 2);
+      const int center_y = lead.bounds.y() + lead.bounds.height() / 2;
+
+      const gfx::FontList& font = glyph->font_list();
+      // 左右各留 4，字比格子宽（W、M 等）时也不被自己的边界裁掉。
+      constexpr int kGlyphOverhang = 4;
+      const int text_width = glyph->GetPreferredSize().width();
+      const int glyph_width =
+          std::max(glyph_slot, text_width) + 2 * kGlyphOverhang;
+      const int slot_center_x = group_x + centered_design_width + gap +
+                                glyph_slot / 2;
+      // 行高等于字体高时 RenderText 不再挪基线，基线就在 GetBaseline()。
+      // 字形中线 = 基线 - 大写字母高 / 2（汉字的字面中线在这个位置上下
+      // 不到 0.4，按整 DIP 摆放时与大写落在同一行）。
+      const int baseline_y = center_y + (font.GetCapHeight() + 1) / 2;
+      layouts.child_layouts.emplace_back(
+          glyph, true,
+          gfx::Rect(slot_center_x - glyph_width / 2,
+                    baseline_y - font.GetBaseline(), glyph_width,
+                    font.GetHeight()));
+    } else {
+      layouts.child_layouts.emplace_back(glyph, false, gfx::Rect());
+    }
+  }
+
   if (TabView().glic_tab_underline_view_) {
     const gfx::Rect glic_bounds =
         gfx::Rect(0, 0, 2 * glic::TabUnderlineView::kEffectThickness, height);
