diff --git a/chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc b/chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc
index 18a885d04d..abdcc889f0 100644
--- a/chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc
+++ b/chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc
@@ -108,6 +108,10 @@
 #include "ui/events/types/event_type.h"
 #include "ui/gfx/animation/slide_animation.h"
 #include "ui/gfx/canvas.h"
+#include "ui/gfx/geometry/transform.h"
+#include "ui/gfx/animation/tween.h"
+#include "ui/compositor/layer.h"
+#include "ui/compositor/layer_solid_color.h"
 #include "ui/gfx/favicon_size.h"
 #include "ui/gfx/geometry/insets.h"
 #include "ui/gfx/geometry/rect.h"
@@ -389,6 +393,10 @@ BookmarkBarView::BookmarkBarView(
   SetProperty(views::kElementIdentifierKey, kBookmarkBarElementId);
 
   size_animation_.Reset(1);
+  // Windrise motion-1115 N8: state change 180ms on cubic-bezier(.23, 1, .32, 1)
+  // (upstream: SlideAnimation default 120ms EASE_OUT, animating the height).
+  size_animation_.SetSlideDuration(base::Milliseconds(180));
+  size_animation_.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
   if (!gfx::Animation::ShouldRenderRichAnimation()) {
     animations_enabled = false;
   }
@@ -464,8 +472,20 @@ void BookmarkBarView::SetBookmarkBarState(
     } else {
       size_animation_.Hide();
     }
+    // Windrise N8: showing takes the full height right away (one reflow of
+    // the web contents), then the bar fades in inside it.
+    if (WindriseHasHeight() != windrise_has_height_) {
+      windrise_has_height_ = WindriseHasHeight();
+      if (browser_view_) {
+        browser_view_->ToolbarSizeChanged(false);
+      }
+    }
+    UpdateWindriseReveal();
   } else {
     size_animation_.Reset(state == BookmarkBar::SHOW ? 1 : 0);
+    // Windrise N8: no animation -> no reveal layer, height follows the state.
+    UpdateWindriseReveal();
+    windrise_has_height_ = WindriseHasHeight();
     if (!animations_enabled) {
       AnimationEnded(&size_animation_);
     }
@@ -631,11 +651,64 @@ gfx::Size BookmarkBarView::CalculatePreferredSize(
     const views::SizeBounds& available_size) const {
   gfx::Size prefsize;
   int preferred_height = GetLayoutConstant(LayoutConstant::kBookmarkBarHeight);
-  prefsize.set_height(
-      static_cast<int>(preferred_height * size_animation_.GetCurrentValue()));
+  // Windrise N8: no per-frame height (that re-laid out the whole window and
+  // resized the web contents every frame). Full height while showing or while
+  // still fading out; 0 once fully hidden.
+  prefsize.set_height(WindriseHasHeight() ? preferred_height : 0);
   return prefsize;
 }
 
+bool BookmarkBarView::WindriseHasHeight() const {
+  return size_animation_.IsShowing() || size_animation_.GetCurrentValue() > 0;
+}
+
+void BookmarkBarView::UpdateWindriseReveal() {
+  const bool animating = size_animation_.is_animating();
+  if (!animating) {
+    if (windrise_reveal_backing_) {
+      // Must leave the region before DestroyLayer() (it DCHECKs that no
+      // layers remain below / above).
+      RemoveLayerFromRegions(windrise_reveal_backing_.get());
+      windrise_reveal_backing_.reset();
+    }
+    if (layer()) {
+      DestroyLayer();
+    }
+    return;
+  }
+  if (!layer()) {
+    // Own layer only while fading (destroyed at the end).
+    SetPaintToLayer();
+    layer()->SetFillsBoundsOpaquely(false);
+  }
+  // Windrise 甲23 §4.3: the strip under the bar never fades. An opaque layer
+  // in the toolbar colour (what ThemedBackground paints here, kBar #EEF0F2)
+  // stays below the bar's layer at full opacity and without the 4px offset;
+  // only the bar's contents fade / slide over it.
+  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
+    if (!windrise_reveal_backing_) {
+      windrise_reveal_backing_ = std::make_unique<ui::LayerSolidColor>();
+      windrise_reveal_backing_->SetName("WindriseBookmarkBarBacking");
+      windrise_reveal_backing_->SetFillsBoundsOpaquely(true);
+      windrise_reveal_backing_->SetBounds(gfx::Rect(size()));
+      AddLayerToRegion(windrise_reveal_backing_.get(),
+                       views::LayerRegion::kBelow);
+    }
+    windrise_reveal_backing_->SetColor(
+        SkColor4f::FromColor(color_provider->GetColor(kColorToolbar)));
+    // Region layers keep their own size (the view only moves them); follow
+    // the bar if the window is resized mid-fade.
+    windrise_reveal_backing_->SetBounds(
+        gfx::Rect(size()) + layer()->bounds().OffsetFromOrigin());
+  }
+  // value: 0 = hidden, 1 = shown (already eased by WINDRISE_EASE_OUT).
+  const double value = size_animation_.GetCurrentValue();
+  layer()->SetOpacity(static_cast<float>(value));
+  gfx::Transform transform;
+  transform.Translate(0, -4.0 * (1.0 - value));
+  layer()->SetTransform(transform);
+}
+
 gfx::Size BookmarkBarView::GetMinimumSize() const {
   // The minimum width of the bookmark bar should at least contain the overflow
   // button, by which one can access all the Bookmark Bar items, and the "All
@@ -694,23 +767,13 @@ void BookmarkBarView::Layout(PassKey) {
   const int button_height =
       GetLayoutConstant(LayoutConstant::kBookmarkBarButtonHeight);
 
-  // Bookmark bar buttons should be centered between the bottom of the location
-  // bar and the bottom of the bookmarks bar, which requires factoring in the
-  // bottom margin of the toolbar into the button position.
-  int toolbar_bottom_margin = 0;
-  // Note: |browser_view_| may be null during tests.
-  if (browser_view_ && !browser_view_->IsFullscreen()) {
-    toolbar_bottom_margin = browser_view_->toolbar()->height() -
-                            browser_view_->GetLocationBar()->Bounds().bottom();
-  }
-  // Center the buttons in the total available space.
-  const int total_height = GetContentsBounds().height() + toolbar_bottom_margin;
-  const auto center_y = [total_height, toolbar_bottom_margin](int height) {
-    const int top_margin = (total_height - height) / 2;
-    // Calculate the top inset in the bookmarks bar itself (not counting the
-    // space in the toolbar) but do not allow the buttons to leave the bookmarks
-    // bar.
-    return std::max(0, top_margin - toolbar_bottom_margin);
+  // Windrise 甲23: center the buttons inside the bookmarks bar itself (frame
+  // 36:26925: 32 tall bar, 28 tall buttons, 2 above and 2 below). Upstream
+  // centered them between the bottom of the location bar and the bottom of
+  // the bar, which here pins them to the bar's top edge (0 above, 4 below).
+  const int total_height = GetContentsBounds().height();
+  const auto center_y = [total_height](int height) {
+    return std::max(0, (total_height - height) / 2);
   };
   const int y = center_y(button_height);
 
@@ -1117,13 +1180,21 @@ void BookmarkBarView::AddedToWidget() {
 }
 
 void BookmarkBarView::AnimationProgressed(const gfx::Animation* animation) {
-  // |browser_view_| can be null during tests.
-  if (browser_view_) {
-    browser_view_->ToolbarSizeChanged(true);
+  // Windrise N8: per frame only the bar's layer opacity / transform changes;
+  // the window is re-laid out only when the height steps.
+  UpdateWindriseReveal();
+  if (WindriseHasHeight() != windrise_has_height_) {
+    windrise_has_height_ = WindriseHasHeight();
+    // |browser_view_| can be null during tests.
+    if (browser_view_) {
+      browser_view_->ToolbarSizeChanged(false);
+    }
   }
 }
 
 void BookmarkBarView::AnimationEnded(const gfx::Animation* animation) {
+  UpdateWindriseReveal();
+  windrise_has_height_ = WindriseHasHeight();
   // |browser_view_| can be null during tests.
   if (browser_view_) {
     browser_view_->ToolbarSizeChanged(false);
