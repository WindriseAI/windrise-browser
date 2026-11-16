diff --git a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h
index e95017e783..c65ac0307a 100644
--- a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h
+++ b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h
@@ -5,6 +5,7 @@
 #ifndef CHROME_BROWSER_UI_VIEWS_TABS_COMMON_TAB_COLLECTION_ANIMATING_LAYOUT_MANAGER_H_
 #define CHROME_BROWSER_UI_VIEWS_TABS_COMMON_TAB_COLLECTION_ANIMATING_LAYOUT_MANAGER_H_
 
+#include <array>
 #include <memory>
 #include <vector>
 
@@ -14,6 +15,7 @@
 #include "base/functional/callback_helpers.h"
 #include "base/memory/raw_ptr.h"
 #include "base/memory/raw_ref.h"
+#include "base/time/time.h"
 #include "ui/gfx/animation/slide_animation.h"
 #include "ui/gfx/geometry/vector2d.h"
 #include "ui/views/animation/animation_delegate_views.h"
@@ -59,7 +61,39 @@ class TabCollectionAnimatingLayoutManager : public views::LayoutManagerBase {
    private:
     bool HasAnimatingManagers() const;
 
+    // Windrise: called right after `animation_.Show()` (re)starts a run.
+    // `from_start` is true for a run that starts from 0 (nothing on screen has
+    // moved yet); such a run first holds at its start, see
+    // WindriseResumeAfterHitch().
+    void WindriseOnRunStarted(double value_from, bool from_start);
+    // Windrise: a run from 0 shows its start position on its first frame and
+    // keeps holding there while frames arrive late (> 1.5 of this machine's
+    // usual frame interval, at most kWindriseMaxHold after Show()), so the
+    // UI-thread stall that follows Ctrl+T / "+" (the new tab's navigation
+    // commits right after the first frame) lands before any movement. Once
+    // moving, a late step advances by one usual frame only instead of jumping
+    // over the part of the curve that was never shown. Returns the value to
+    // use.
+    double WindriseResumeAfterHitch(double value);
+    // Windrise: median of the recent tick intervals (the machine's usual frame
+    // interval), or one 60Hz frame until enough intervals were seen. A median
+    // ignores the odd stall, so a steadily slower machine is not mistaken for
+    // one that stalls on every frame.
+    base::TimeDelta WindriseFrameInterval() const;
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
+    std::array<base::TimeDelta, 15> windrise_intervals_;
+    size_t windrise_interval_count_ = 0;
+    size_t windrise_interval_next_ = 0;
     std::vector<raw_ptr<TabCollectionAnimatingLayoutManager>> managers_;
     // Where in the animation the last layout recalculation happened.
     double starting_offset_ = 0.0;
