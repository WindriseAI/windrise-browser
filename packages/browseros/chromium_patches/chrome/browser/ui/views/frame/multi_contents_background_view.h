diff --git a/chrome/browser/ui/views/frame/multi_contents_background_view.h b/chrome/browser/ui/views/frame/multi_contents_background_view.h
index 1328eaffd9..94aea0d705 100644
--- a/chrome/browser/ui/views/frame/multi_contents_background_view.h
+++ b/chrome/browser/ui/views/frame/multi_contents_background_view.h
@@ -27,6 +27,11 @@ class MultiContentsBackgroundView : public views::View {
   ~MultiContentsBackgroundView() override;
 
   void SetRoundedCorners(const gfx::RoundedCornersF& radii);
+
+  // 风桥 N10：单窗口态（非分屏）网页区带圆角时，角上要露出窗口底色 #DDE1E5
+  // （kColorWindriseWebContentsGap），而不是后面的工具栏色。开着时本视图用这个
+  // 颜色画成不圆角的实色底（藏在圆角网页区后面）；分屏时关掉，保持上游行为。
+  void SetUseWindowGapColor(bool use);
   const gfx::RoundedCornersF& GetRoundedCorners() const;
 
   // views::View:
@@ -35,10 +40,12 @@ class MultiContentsBackgroundView : public views::View {
 
  private:
   ui::LayerType CalculateLayerType() const;
+  void RefreshLayerAndColor();
 
   void UpdateSolidLayerColor();
 
   const raw_ptr<BrowserView> browser_view_;
+  bool use_window_gap_color_ = false;
 };
 
 #endif  // CHROME_BROWSER_UI_VIEWS_FRAME_MULTI_CONTENTS_BACKGROUND_VIEW_H_
