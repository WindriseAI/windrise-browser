diff --git a/chrome/browser/ui/views/frame/multi_contents_view.h b/chrome/browser/ui/views/frame/multi_contents_view.h
index e39f000c2a..d6e0e32375 100644
--- a/chrome/browser/ui/views/frame/multi_contents_view.h
+++ b/chrome/browser/ui/views/frame/multi_contents_view.h
@@ -287,10 +287,17 @@ class MultiContentsView
   // Helper function that gets the component of |size| along the resize axis.
   int GetResizeAxisComponent(const gfx::Size& size) const;
 
+  // 风桥 N10：按「是否分屏」同步背景视图的圆角与颜色，变了才重新布局。
+  void UpdateBackgroundView();
+
   raw_ptr<BrowserView> browser_view_;
   std::unique_ptr<MultiContentsViewDelegate> delegate_;
 
   raw_ptr<MultiContentsBackgroundView> background_view_;
+  // 风桥 N10：网页区圆角基准。单窗口态背景视图不加圆角（用窗口底色填满，露在
+  // 圆角网页区的角上），所以圆角不能再从背景视图的 layer 里读回来，自己存一份。
+  gfx::RoundedCornersF background_radii_;
+  bool background_gap_mode_ = false;
 
   // Holds ContentsContainerViews, when not in a split view the second
   // ContentsContainerView is not visible.
