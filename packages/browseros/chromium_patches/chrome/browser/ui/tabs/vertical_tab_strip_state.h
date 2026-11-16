diff --git a/chrome/browser/ui/tabs/vertical_tab_strip_state.h b/chrome/browser/ui/tabs/vertical_tab_strip_state.h
index 1cc98a4df8..8841a5e8dc 100644
--- a/chrome/browser/ui/tabs/vertical_tab_strip_state.h
+++ b/chrome/browser/ui/tabs/vertical_tab_strip_state.h
@@ -10,11 +10,15 @@ namespace tabs {
 inline constexpr int kVerticalTabStripDefaultUncollapsedWidth = 240;
 // TODO(crbug.com/465833741): Replace constant with derived value based on
 // caption buttons.
-inline constexpr int kVerticalTabStripUncollapsedMinWidth = 126;
+// 风桥：方案标注表第 1 行，展开最小宽 200（上游 126）。
+inline constexpr int kVerticalTabStripUncollapsedMinWidth = 200;
 // TODO(crbug.com/465832180): Replace constant based width final max width for
 // view.
 inline constexpr int kVerticalTabStripUncollapsedMaxWidth = 400;
-inline constexpr int kVerticalTabStripCollapsedWidth = 56;
+// 风桥：折叠宽 = 上游原宽 56 + 一个汉字 14 = 70（theme-v2 §3「以真实常量为准」，
+// 协调人 1116 裁定）。标签宽 70 - 2×8 = 54，放得下「图标 16 + 间距 5 + 字 14」
+// = 35，一组居中后图标 x=10，和展开时图标的 x 相同。原来是 66。
+inline constexpr int kVerticalTabStripCollapsedWidth = 70;
 // TODO(crbug.com/465833741): Determine snapping behavior.
 inline constexpr int kVerticalTabStripCollapseSnapWidth =
     (kVerticalTabStripUncollapsedMinWidth + kVerticalTabStripCollapsedWidth) /
