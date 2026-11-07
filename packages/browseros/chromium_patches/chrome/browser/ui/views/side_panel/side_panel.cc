diff --git a/chrome/browser/ui/views/side_panel/side_panel.cc b/chrome/browser/ui/views/side_panel/side_panel.cc
index 0098885..7abcb3c 100644
--- a/chrome/browser/ui/views/side_panel/side_panel.cc
+++ b/chrome/browser/ui/views/side_panel/side_panel.cc
@@ -125,7 +125,7 @@ class ContentParentBackground : public views::Background {
         view->GetLocalBounds(), get_rounded_corners_callback_.Run()));
     canvas->ClipPath(path, /*do_anti_alias=*/true);
 
-      ThemedBackground::PaintBackground(canvas, view, browser_view_);
+    ThemedBackground::PaintBackground(canvas, view, browser_view_);
   }
 
  private:
@@ -657,8 +657,10 @@ double SidePanel::GetAnimationValueFor(BrowserAnimationSequence which) const {
 }
 
 bool SidePanel::ShouldShowAnimation() const {
-  bool should_show_animations =
-      gfx::Animation::ShouldRenderRichAnimation() && !animations_disabled_;
+  // BrowserOS: animations_disabled_browseros_ used to control animation
+  bool should_show_animations = gfx::Animation::ShouldRenderRichAnimation() &&
+                                !animations_disabled_ &&
+                                animations_disabled_browseros_;
   return should_show_animations;
 }
 
