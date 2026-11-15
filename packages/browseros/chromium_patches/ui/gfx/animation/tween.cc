diff --git a/ui/gfx/animation/tween.cc b/ui/gfx/animation/tween.cc
index 5fcd1e2543..9a235b5e21 100644
--- a/ui/gfx/animation/tween.cc
+++ b/ui/gfx/animation/tween.cc
@@ -187,6 +187,11 @@ double Tween::CalculateValue(Tween::Type type, double state) {
       static const gfx::CubicBezier bezier(0.05, 0.7, 0.1, 1);
       return bezier.Solve(state);
     }
+
+    case WINDRISE_EASE_OUT: {
+      static const gfx::CubicBezier bezier(0.23, 1, 0.32, 1);
+      return bezier.Solve(state);
+    }
   }
 
   NOTREACHED();
