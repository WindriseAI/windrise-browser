diff --git a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.cc b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.cc
index b2158e2ccc..02fc626d9f 100644
--- a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.cc
+++ b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.cc
@@ -94,12 +94,29 @@ bool TabCollectionAnimatingLayoutManager::Delegate::ShouldAnimateLayout() {
   return true;
 }
 
+// Windrise motion tokens: tab add / close / reorder / group changes are a
+// state change -> 180ms on cubic-bezier(.23, 1, .32, 1) (gfx::Tween
+// WINDRISE_EASE_OUT). Upstream was 200ms EASE_IN_OUT. Retargeting from the
+// current layout and RichAnimationDuration (reduced motion -> 0) are unchanged.
+constexpr int kWindriseTabLayoutMs = 180;
+
+// Windrise hitch guard. The animation is time based, so a UI-thread stall
+// right after the first frame (Ctrl+T / "+" builds the new tab's contents
+// there) used to skip most of the strong ease-out unseen: a small move, a
+// freeze, then a jump to the end. A step longer than 1.5 frames at 60Hz now
+// advances by one frame only, so the motion resumes from what was shown. On
+// normal frames (60 / 120Hz) nothing changes and progress stays time based.
+// After twice the duration the guard lets go so a slow machine still finishes.
+constexpr base::TimeDelta kWindriseHitchGap = base::Microseconds(25000);
+constexpr base::TimeDelta kWindriseResumeStep = base::Microseconds(16667);
+constexpr int kWindriseMaxStretch = 2;
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
@@ -154,8 +171,9 @@ void TabCollectionAnimatingLayoutManager::AnimationCoordinator::
     }
     animation_.Reset(0.0);
     animation_.SetSlideDuration(
-        gfx::Animation::RichAnimationDuration(base::Milliseconds(200)));
+        gfx::Animation::RichAnimationDuration(base::Milliseconds(kWindriseTabLayoutMs)));
     animation_.Show();
+    WindriseOnRunStarted(0.0);
   } else {
     // We are still early in the animation. Simply update the starting offset
     // and re-baseline all active managers to `current_offset_`. The animation
@@ -168,7 +186,9 @@ void TabCollectionAnimatingLayoutManager::AnimationCoordinator::
       }
     }
     if (!animation_.is_animating()) {
+      const double value_from = animation_.GetCurrentValue();
       animation_.Show();
+      WindriseOnRunStarted(value_from);
     }
   }
 }
@@ -197,10 +217,11 @@ TabCollectionAnimatingLayoutManager::AnimationCoordinator::GetCurrentProgress()
 
 void TabCollectionAnimatingLayoutManager::AnimationCoordinator::
     AnimationProgressed(const gfx::Animation* animation) {
-  if (current_offset_ == animation->GetCurrentValue()) {
+  const double value = WindriseResumeAfterHitch(animation->GetCurrentValue());
+  if (current_offset_ == value) {
     return;
   }
-  current_offset_ = animation->GetCurrentValue();
+  current_offset_ = value;
   for (TabCollectionAnimatingLayoutManager* manager : managers_) {
     if (manager->is_animating_) {
       manager->OnAnimationProgressed();
@@ -227,6 +248,50 @@ void TabCollectionAnimatingLayoutManager::AnimationCoordinator::AnimationEnded(
   }
 }
 
+void TabCollectionAnimatingLayoutManager::AnimationCoordinator::
+    WindriseOnRunStarted(double value_from) {
+  // Mirrors SlideAnimation::GetDuration(): a run that starts partway only
+  // covers the remaining distance, in proportionally less time.
+  windrise_run_duration_ =
+      animation_.is_animating()
+          ? animation_.GetSlideDuration() * (1.0 - value_from)
+          : base::TimeDelta();
+  windrise_value_from_ = value_from;
+  windrise_linear_ = 0.0;
+  windrise_run_start_ = base::TimeTicks::Now();
+  windrise_last_step_ = windrise_run_start_;
+}
+
+double TabCollectionAnimatingLayoutManager::AnimationCoordinator::
+    WindriseResumeAfterHitch(double value) {
+  // Reduced motion (zero duration) reports progress synchronously from Show()
+  // without a running animation; leave that and any stale run untouched.
+  if (!animation_.is_animating() || windrise_run_duration_.is_zero() ||
+      windrise_last_step_.is_null()) {
+    return value;
+  }
+  const base::TimeTicks now = base::TimeTicks::Now();
+  const base::TimeDelta step = now - windrise_last_step_;
+  windrise_last_step_ = now;
+  double linear = windrise_linear_ + step / windrise_run_duration_;
+  if (step > kWindriseHitchGap &&
+      now - windrise_run_start_ <
+          windrise_run_duration_ * kWindriseMaxStretch) {
+    // Pull the animation clock back so this step only shows one frame of
+    // progress; the following steps continue from here on time.
+    linear = windrise_linear_ + kWindriseResumeStep / windrise_run_duration_;
+    if (linear < 1.0) {
+      animation_.SetCurrentValue(linear);
+      value = windrise_value_from_ +
+              (1.0 - windrise_value_from_) *
+                  gfx::Tween::CalculateValue(gfx::Tween::WINDRISE_EASE_OUT,
+                                             linear);
+    }
+  }
+  windrise_linear_ = std::min(linear, 1.0);
+  return value;
+}
+
 bool TabCollectionAnimatingLayoutManager::AnimationCoordinator::
     HasAnimatingManagers() const {
   return std::ranges::any_of(
