diff --git a/chrome/browser/ui/views/toolbar/toolbar_view.h b/chrome/browser/ui/views/toolbar/toolbar_view.h
index e38c93e2c6..4c49793d05 100644
--- a/chrome/browser/ui/views/toolbar/toolbar_view.h
+++ b/chrome/browser/ui/views/toolbar/toolbar_view.h
@@ -66,6 +66,7 @@ class ToolbarController;
 class ToolbarDivider;
 class OverflowButton;
 class PerformanceInterventionButton;
+class WindriseGridButton;
 
 namespace views {
 class FlexLayout;
@@ -455,6 +456,9 @@ class ToolbarView : public views::AccessiblePaneView,
 
   raw_ptr<ToolbarButton> ai_overlay_dialog_button_ = nullptr;
 
+  // 风桥「格子」按钮：地址栏之后、扩展按钮之前。
+  raw_ptr<WindriseGridButton> windrise_grid_button_ = nullptr;
+
   const raw_ptr<BrowserWindowInterface> browser_;
   const raw_ptr<BrowserView> browser_view_;
   base::WeakPtr<glic::GlicSplitButtonController> glic_split_button_controller_;
