diff --git a/chrome/browser/ui/views/tabs/common/dragged_tabs_container.cc b/chrome/browser/ui/views/tabs/common/dragged_tabs_container.cc
index 1fcbfddf16..b6689be87d 100644
--- a/chrome/browser/ui/views/tabs/common/dragged_tabs_container.cc
+++ b/chrome/browser/ui/views/tabs/common/dragged_tabs_container.cc
@@ -27,7 +27,10 @@
 #include "ui/gfx/geometry/vector2d.h"
 #include "ui/views/controls/scroll_view.h"
 #include "ui/views/layout/proposed_layout.h"
+#include "ui/gfx/animation/animation_container.h"
+#include "ui/views/animation/compositor_animation_runner.h"
 #include "ui/views/view.h"
+#include "ui/views/widget/widget.h"
 #include "ui/views/view_utils.h"
 
 namespace {
@@ -261,12 +264,14 @@ void DraggedTabsContainer::InitializeDragStartAnimation(
   CHECK(animating_views_start_offsets_.empty());
 
   // Duration of the animation for dragged views to become contiguous.
+  // Windrise motion tokens: state change 180ms on cubic-bezier(.23, 1, .32, 1)
+  // (upstream: 200ms EASE_IN_OUT).
   static constexpr base::TimeDelta kDragStartAnimationDuration =
-      base::Milliseconds(200);
+      base::Milliseconds(180);
   base::TimeDelta drag_start_animation_duration =
       gfx::Animation::RichAnimationDuration(kDragStartAnimationDuration);
   static constexpr gfx::Tween::Type kStartDragAnimationTweenType =
-      gfx::Tween::Type::EASE_IN_OUT;
+      gfx::Tween::Type::WINDRISE_EASE_OUT;
 
   const auto& drag_handler = GetDragHandler();
   base::TimeDelta drag_time_elapsed =
@@ -321,6 +326,18 @@ void DraggedTabsContainer::InitializeDragStartAnimation(
   drag_start_animation_.SetTweenType(kStartDragAnimationTweenType);
   drag_start_animation_.SetSlideDuration(drag_start_animation_duration);
 
+  // Windrise: `drag_start_animation_` is driven by a plain gfx::AnimationDelegate,
+  // i.e. by AnimationContainer's default timer (16.6ms, never faster than
+  // 10ms), which caps it at 60fps on a 120Hz display. Step it from the
+  // widget's compositor instead (display-synced, like AnimationDelegateViews).
+  if (views::Widget* widget = host_view_->GetWidget();
+      widget && widget->GetCompositor()) {
+    auto container = base::MakeRefCounted<gfx::AnimationContainer>();
+    container->SetAnimationRunner(
+        std::make_unique<views::CompositorAnimationRunner>(widget, FROM_HERE));
+    drag_start_animation_.SetContainer(container.get());
+  }
+
   // Set the animations value to be proportional according to the time that
   // elapsed since the drag started. This makes the transition between
   // containers smoother.
