diff --git a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.cc b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.cc
index b2158e2ccc..880cb0a0e7 100644
--- a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.cc
+++ b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.cc
@@ -94,12 +94,18 @@ bool TabCollectionAnimatingLayoutManager::Delegate::ShouldAnimateLayout() {
   return true;
 }
 
+// Windrise motion tokens: tab add / close / reorder / group changes are a
+// state change -> 180ms on cubic-bezier(.23, 1, .32, 1) (gfx::Tween
+// WINDRISE_EASE_OUT). Upstream was 200ms EASE_IN_OUT. Retargeting from the
+// current layout and RichAnimationDuration (reduced motion -> 0) are unchanged.
+constexpr int kWindriseTabLayoutMs = 180;
+
 TabCollectionAnimatingLayoutManager::AnimationCoordinator::AnimationCoordinator(
     views::View& view)
     : views::AnimationDelegateViews(&view) {
   animation_.SetSlideDuration(
-      gfx::Animation::RichAnimationDuration(base::Milliseconds(200)));
-  animation_.SetTweenType(gfx::Tween::EASE_IN_OUT);
+      gfx::Animation::RichAnimationDuration(base::Milliseconds(kWindriseTabLayoutMs)));
+  animation_.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
 }
 
 TabCollectionAnimatingLayoutManager::AnimationCoordinator::
@@ -154,7 +160,7 @@ void TabCollectionAnimatingLayoutManager::AnimationCoordinator::
     }
     animation_.Reset(0.0);
     animation_.SetSlideDuration(
-        gfx::Animation::RichAnimationDuration(base::Milliseconds(200)));
+        gfx::Animation::RichAnimationDuration(base::Milliseconds(kWindriseTabLayoutMs)));
     animation_.Show();
   } else {
     // We are still early in the animation. Simply update the starting offset
