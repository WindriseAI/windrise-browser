diff --git a/chrome/browser/ui/views/bookmarks/bookmark_bar_view.h b/chrome/browser/ui/views/bookmarks/bookmark_bar_view.h
index 1ecd6d72fe..9c1f560143 100644
--- a/chrome/browser/ui/views/bookmarks/bookmark_bar_view.h
+++ b/chrome/browser/ui/views/bookmarks/bookmark_bar_view.h
@@ -60,6 +60,10 @@ namespace tab_groups {
 class SavedTabGroupBar;
 }
 
+namespace ui {
+class LayerSolidColor;
+}  // namespace ui
+
 namespace views {
 class MenuButton;
 class MenuItemView;
@@ -495,6 +499,17 @@ class BookmarkBarView : public views::AccessiblePaneView,
   // Animation controlling showing and hiding of the bar.
   gfx::SlideAnimation size_animation_{this};
 
+  // Windrise motion-1115 N8: the bar's height is a step (full while showing or
+  // still fading out, 0 otherwise), so the web contents reflow exactly once;
+  // the bar itself fades + slides 4px inside the space it already owns.
+  bool WindriseHasHeight() const;
+  void UpdateWindriseReveal();
+  bool windrise_has_height_ = true;
+  // Windrise 甲23 §4.3: while the bar fades, an opaque bar-coloured layer sits
+  // below the bar's own layer (not faded, not moved), so only the bar's
+  // contents fade and slide; the strip itself stays toolbar colour every frame.
+  std::unique_ptr<ui::LayerSolidColor> windrise_reveal_backing_;
+
   BookmarkBar::State bookmark_bar_state_ = BookmarkBar::SHOW;
 
   base::ObserverList<BookmarkBarViewObserver>::Unchecked observers_;
