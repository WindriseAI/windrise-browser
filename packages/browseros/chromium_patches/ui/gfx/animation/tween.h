diff --git a/ui/gfx/animation/tween.h b/ui/gfx/animation/tween.h
index 917871dafc..a0433c096a 100644
--- a/ui/gfx/animation/tween.h
+++ b/ui/gfx/animation/tween.h
@@ -107,6 +107,12 @@ class ANIMATION_EXPORT Tween {
 
     ACCEL_5_70_DECEL_90,  // Start at peak velocity and very soft
                           // deceleration.
+
+    // Windrise: the one motion curve shared with the desktop app,
+    // cubic-bezier(.23, 1, .32, 1) (strong ease-out). Every Windrise browser
+    // animation uses this; durations come from the same token set
+    // (press 120ms, state 180ms, panel/enter 240ms, fade 400ms).
+    WINDRISE_EASE_OUT,
   };
 
   Tween(const Tween&) = delete;
