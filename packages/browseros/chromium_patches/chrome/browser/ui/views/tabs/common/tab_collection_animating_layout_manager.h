diff --git a/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h b/chrome/browser/ui/views/tabs/common/tab_collection_animating_layout_manager.h
index e95017e783..b2f4fb60e5 100644
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
@@ -59,7 +60,21 @@ class TabCollectionAnimatingLayoutManager : public views::LayoutManagerBase {
    private:
     bool HasAnimatingManagers() const;
 
+    // Windrise: called right after `animation_.Show()` (re)starts a run.
+    void WindriseOnRunStarted(double value_from);
+    // Windrise: when the UI thread was blocked for longer than 1.5 frames
+    // (e.g. Ctrl+T building the new tab's contents right after the first
+    // frame), advance this step by one frame only instead of jumping over the
+    // part of the curve that was never shown. Returns the value to use.
+    double WindriseResumeAfterHitch(double value);
+
     gfx::SlideAnimation animation_{this};
+    // Windrise hitch guard state, see WindriseResumeAfterHitch().
+    base::TimeTicks windrise_run_start_;
+    base::TimeTicks windrise_last_step_;
+    base::TimeDelta windrise_run_duration_;
+    double windrise_value_from_ = 0.0;
+    double windrise_linear_ = 0.0;
     std::vector<raw_ptr<TabCollectionAnimatingLayoutManager>> managers_;
     // Where in the animation the last layout recalculation happened.
     double starting_offset_ = 0.0;
