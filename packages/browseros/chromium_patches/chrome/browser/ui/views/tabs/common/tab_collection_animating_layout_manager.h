diff --git a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h
index e95017e783..bd6de76148 100644
--- a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h
+++ b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h
@@ -14,6 +14,7 @@
 #include "base/functional/callback_helpers.h"
 #include "base/memory/raw_ptr.h"
 #include "base/memory/raw_ref.h"
+#include "base/time/time.h"
 #include "ui/gfx/animation/slide_animation.h"
 #include "ui/gfx/geometry/vector2d.h"
 #include "ui/views/animation/animation_delegate_views.h"
@@ -59,7 +60,30 @@ class TabCollectionAnimatingLayoutManager : public views::LayoutManagerBase {
    private:
     bool HasAnimatingManagers() const;
 
+    // Windrise: called right after `animation_.Show()` (re)starts a run.
+    // `from_start` is true for a run that starts from 0 (nothing on screen has
+    // moved yet); such a run first holds at its start, see
+    // WindriseResumeAfterHitch().
+    void WindriseOnRunStarted(double value_from, bool from_start);
+    // Windrise: a run from 0 shows its start position on its first frame and
+    // keeps holding there while frames arrive late (> 1.5 frames at 60Hz, at
+    // most kWindriseMaxHold after Show()), so the UI-thread stall that follows
+    // Ctrl+T / "+" (the new tab's navigation commits right after the first
+    // frame) lands before any movement. Once moving, a late step advances by
+    // one frame only instead of jumping over the part of the curve that was
+    // never shown. Returns the value to use.
+    double WindriseResumeAfterHitch(double value);
+
     gfx::SlideAnimation animation_{this};
+    // Windrise hitch guard state, see WindriseResumeAfterHitch().
+    base::TimeTicks windrise_run_start_;
+    base::TimeTicks windrise_last_step_;
+    base::TimeTicks windrise_hold_until_;
+    base::TimeDelta windrise_run_duration_;
+    double windrise_value_from_ = 0.0;
+    double windrise_linear_ = 0.0;
+    bool windrise_holding_ = false;
+    bool windrise_first_tick_ = false;
     std::vector<raw_ptr<TabCollectionAnimatingLayoutManager>> managers_;
     // Where in the animation the last layout recalculation happened.
     double starting_offset_ = 0.0;
