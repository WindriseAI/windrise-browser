diff --git a/chrome/browser/ui/views/animations/side_panel_animations.cc b/chrome/browser/ui/views/animations/side_panel_animations.cc
index ba55a4536c..f7680dcf37 100644
--- a/chrome/browser/ui/views/animations/side_panel_animations.cc
+++ b/chrome/browser/ui/views/animations/side_panel_animations.cc
@@ -48,23 +48,24 @@ SidePanelAnimations::~SidePanelAnimations() = default;
 
 SidePanelAnimations::GroupInfos SidePanelAnimations::GenerateAnimations()
     const {
-  constexpr int kDefaultAnimationMs = 350;
-  const bool use_flyover =
-      browser_animations::GetPanelAnimationType() ==
-      browser_animations::PanelAnimationType::kClipAndSingleReflow;
-  const gfx::Tween::Type tween = use_flyover
-                                     ? gfx::Tween::Type::ACCEL_80_DECEL_20
-                                     : gfx::Tween::Type::ACCEL_45_DECEL_88;
+  // Windrise motion tokens (same as the desktop app): open 240ms (panel
+  // expand), close 180ms (state change), one curve cubic-bezier(.23, 1, .32, 1).
+  // Upstream: 350ms both ways, ACCEL_80_DECEL_20 on Windows. The shadow
+  // segments keep upstream's proportions (150+100 of 350 -> 103+69 of 240;
+  // 100 of 350 -> 51 of 180). Clip + single reflow is unchanged.
+  constexpr int kWindriseOpenMs = 240;
+  constexpr int kWindriseCloseMs = 180;
+  const gfx::Tween::Type tween = gfx::Tween::Type::WINDRISE_EASE_OUT;
   const auto show_shadow_sequence =
       Sequence(kMainAreaShadow, StartingValue(0.0),
-               Segment(StartMs(150), LengthMs(100), ToValue(1.0)));
+               Segment(StartMs(103), LengthMs(69), ToValue(1.0)));
   const auto hide_shadow_sequence =
       Sequence(kMainAreaShadow, StartingValue(1.0),
-               Segment(StartMs(0), LengthMs(100), ToValue(0.0)));
+               Segment(StartMs(0), LengthMs(51), ToValue(0.0)));
 
   return Groups(
       Group(kSidePanel,
-            Motion(kOpen, TotalDurationMs(kDefaultAnimationMs), tween,
+            Motion(kOpen, TotalDurationMs(kWindriseOpenMs), tween,
                    Animate(kPanelWidth, FromValue(0.0), ToValue(1.0)),
                    show_shadow_sequence),
             Motion(kOpenWithContentTransition, TotalDurationMs(400),
@@ -82,7 +83,7 @@ SidePanelAnimations::GroupInfos SidePanelAnimations::GenerateAnimations()
                             Segment(StartMs(100), LengthMs(300), ToValue(0.0),
                                     gfx::Tween::Type::ACCEL_5_70_DECEL_90)),
                    show_shadow_sequence),
-            Motion(kClose, TotalDurationMs(kDefaultAnimationMs), tween,
+            Motion(kClose, TotalDurationMs(kWindriseCloseMs), tween,
                    Animate(kPanelWidth, FromValue(1.0), ToValue(0.0)),
                    hide_shadow_sequence)));
 }
