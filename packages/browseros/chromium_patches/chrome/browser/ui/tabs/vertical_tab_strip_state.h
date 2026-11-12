diff --git a/chrome/browser/ui/tabs/vertical_tab_strip_state.h b/chrome/browser/ui/tabs/vertical_tab_strip_state.h
index 1cc98a4df8..59135c3e64 100644
--- a/chrome/browser/ui/tabs/vertical_tab_strip_state.h
+++ b/chrome/browser/ui/tabs/vertical_tab_strip_state.h
@@ -10,11 +10,13 @@ namespace tabs {
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
+// 风桥：折叠宽 66（上游 56），图标 16 + 首字的折叠条用。
+inline constexpr int kVerticalTabStripCollapsedWidth = 66;
 // TODO(crbug.com/465833741): Determine snapping behavior.
 inline constexpr int kVerticalTabStripCollapseSnapWidth =
     (kVerticalTabStripUncollapsedMinWidth + kVerticalTabStripCollapsedWidth) /
