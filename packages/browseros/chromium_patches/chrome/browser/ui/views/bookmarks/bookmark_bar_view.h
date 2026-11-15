diff --git a/chrome/browser/ui/views/bookmarks/bookmark_bar_view.h b/chrome/browser/ui/views/bookmarks/bookmark_bar_view.h
index 1ecd6d72fe..530ad0c74b 100644
--- a/chrome/browser/ui/views/bookmarks/bookmark_bar_view.h
+++ b/chrome/browser/ui/views/bookmarks/bookmark_bar_view.h
@@ -495,6 +495,13 @@ class BookmarkBarView : public views::AccessiblePaneView,
   // Animation controlling showing and hiding of the bar.
   gfx::SlideAnimation size_animation_{this};
 
+  // Windrise motion-1115 N8: the bar's height is a step (full while showing or
+  // still fading out, 0 otherwise), so the web contents reflow exactly once;
+  // the bar itself fades + slides 4px inside the space it already owns.
+  bool WindriseHasHeight() const;
+  void UpdateWindriseReveal();
+  bool windrise_has_height_ = true;
+
   BookmarkBar::State bookmark_bar_state_ = BookmarkBar::SHOW;
 
   base::ObserverList<BookmarkBarViewObserver>::Unchecked observers_;
