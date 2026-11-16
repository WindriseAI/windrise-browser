diff --git a/chrome/browser/ui/views/tabs/common/tab_view.h b/chrome/browser/ui/views/tabs/common/tab_view.h
index 94398daa53..2e7ff17a6f 100644
--- a/chrome/browser/ui/views/tabs/common/tab_view.h
+++ b/chrome/browser/ui/views/tabs/common/tab_view.h
@@ -46,6 +46,10 @@ namespace glic {
 class TabUnderlineView;
 }  // namespace glic
 
+namespace views {
+class Label;
+}  // namespace views
+
 // The view class for the tab. It is responsible for painting the
 // tab background and displaying the favicon, title, alert indicators and close
 // button. It handles data changed event and view hierarchy changes to updates
@@ -183,6 +187,9 @@ class TabView : public views::View,
   void UpdateTabData(const tabs::TabData& tab);
 
   void UpdateTitle(std::u16string title, bool should_render_loading_title);
+  // 风桥 theme-v2 §4：折叠窄条上露出的那一个字（只有竖向标签有）。
+  void UpdateRailGlyph();
+  void UpdateRailGlyphFont();
   void UpdateBorder();
   void UpdateColors();
   void UpdateContrastRatioValues();
@@ -227,6 +234,9 @@ class TabView : public views::View,
   const raw_ptr<AlertIndicatorButton> alert_indicator_;
   const raw_ptr<TabCloseButton> close_button_;
   raw_ptr<glic::TabUnderlineView> glic_tab_underline_view_ = nullptr;
+  // 风桥：折叠窄条上图标右边的那一个字。只在竖向标签上创建，横向为空。
+  // 显示与否、位置都由 TabViewVerticalLayout 在每次排版时决定。
+  raw_ptr<views::Label> rail_glyph_ = nullptr;
 
   base::CallbackListSubscription node_destroyed_subscription_;
   base::CallbackListSubscription tab_state_changed_subscription_;
