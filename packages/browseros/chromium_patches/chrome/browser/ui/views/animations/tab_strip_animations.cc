diff --git a/chrome/browser/ui/views/animations/tab_strip_animations.cc b/chrome/browser/ui/views/animations/tab_strip_animations.cc
index 6460d13872..596492e1ab 100644
--- a/chrome/browser/ui/views/animations/tab_strip_animations.cc
+++ b/chrome/browser/ui/views/animations/tab_strip_animations.cc
@@ -54,14 +54,23 @@ TabStripAnimations::TabStripAnimations() {
 TabStripAnimations::~TabStripAnimations() = default;
 
 TabStripAnimations::GroupInfos TabStripAnimations::GenerateAnimations() const {
-  const bool use_flyover =
+  [[maybe_unused]] const bool use_flyover =
       browser_animations::GetPanelAnimationType() ==
       browser_animations::PanelAnimationType::kClipAndSingleReflow;
-  const int expand_duration_ms = use_flyover ? 300 : 400;
-  const int collapse_duration_ms = use_flyover ? 250 : 350;
-  const gfx::Tween::Type expand_collapse_tween =
-      use_flyover ? gfx::Tween::ACCEL_80_DECEL_20
-                  : gfx::Tween::EASE_IN_OUT_EMPHASIZED;
+  // Windrise motion tokens (same as the desktop app): panel expand 240ms,
+  // collapse at the state-change step 180ms (collapsing is faster than
+  // expanding), one curve cubic-bezier(.23, 1, .32, 1) everywhere.
+  // `use_flyover` (clip + single reflow of the web contents) is unchanged.
+  const int expand_duration_ms = 240;
+  const int collapse_duration_ms = 180;
+  const gfx::Tween::Type expand_collapse_tween = gfx::Tween::WINDRISE_EASE_OUT;
+  // Hover fly-out uses the same tokens: show 240ms, hide 180ms. The opacity
+  // segment keeps upstream's proportion (100ms of 250ms -> 96ms of 240ms).
+  constexpr int kWindriseFlyoutShowMs = 240;
+  constexpr int kWindriseFlyoutHideMs = 180;
+  constexpr int kWindriseFlyoutFadeMs = 96;
+  constexpr gfx::Tween::Type kWindriseFlyoutTween =
+      gfx::Tween::WINDRISE_EASE_OUT;
   // The sequence goes: one corner disappears, the top animates, and then the
   // other corner appears. These are the percentages of the animation at which
   // these changes happen.
@@ -86,9 +95,9 @@ TabStripAnimations::GroupInfos TabStripAnimations::GenerateAnimations() const {
              Sequence(kTabStripHoverOpacity, StartingValue(1.0),
                       Transition::kStartAtOldValue,
                       Segment(StartMs(expand_duration_ms -
-                                      browser_animations::kFlyoutFadeMs),
+                                      kWindriseFlyoutFadeMs),
                               EndMs(expand_duration_ms), ToValue(0.0),
-                              browser_animations::kFlyoutFadeOutTween))),
+                              gfx::Tween::WINDRISE_EASE_OUT))),
       Motion(kCollapse, TotalDurationMs(collapse_duration_ms),
              expand_collapse_tween,
              Animate(kTabStripWidth, FromValue(1.0), ToValue(0.0)),
@@ -107,27 +116,27 @@ TabStripAnimations::GroupInfos TabStripAnimations::GenerateAnimations() const {
       // the animation and so shares the same animation parameters across all
       // the supported platforms.
       Motion(
-          kExpandOnHover, TotalDurationMs(browser_animations::kFlyoutShowMs),
-          browser_animations::kFlyoutTween,
+          kExpandOnHover, TotalDurationMs(kWindriseFlyoutShowMs),
+          kWindriseFlyoutTween,
           Animate(kTabStripHoverWidth, FromValue(0.0), ToValue(1.0)),
           Animate(kTopCorner, FromValue(DefaultValue()), ToValue(-1.0)),
           Animate(kBottomCorner, FromValue(1.0), ToValue(-1.0)),
           Sequence(
               kTabStripHoverOpacity, StartingValue(0.0),
               Transition::kStartAtOldValue,
-              Segment(StartMs(0), EndMs(browser_animations::kFlyoutFadeMs),
-                      ToValue(1.0), browser_animations::kFlyoutFadeInTween))),
+              Segment(StartMs(0), EndMs(kWindriseFlyoutFadeMs),
+                      ToValue(1.0), gfx::Tween::WINDRISE_EASE_OUT))),
       Motion(kCollapseOnHover,
-             TotalDurationMs(browser_animations::kFlyoutHideMs),
-             browser_animations::kFlyoutTween,
+             TotalDurationMs(kWindriseFlyoutHideMs),
+             kWindriseFlyoutTween,
              Animate(kTabStripHoverWidth, FromValue(1.0), ToValue(0.0)),
              Animate(kTopCorner, FromValue(-1.0), ToValue(DefaultValue())),
              Animate(kBottomCorner, FromValue(-1.0), ToValue(1.0)),
              Sequence(
                  kTabStripHoverOpacity, StartingValue(1.0),
                  Transition::kStartAtOldValue,
-                 Segment(StartMs(browser_animations::kFlyoutHideMs -
-                                 browser_animations::kFlyoutFadeMs),
-                         EndMs(browser_animations::kFlyoutHideMs), ToValue(0.0),
-                         browser_animations::kFlyoutFadeOutTween)))));
+                 Segment(StartMs(kWindriseFlyoutHideMs -
+                                 kWindriseFlyoutFadeMs),
+                         EndMs(kWindriseFlyoutHideMs), ToValue(0.0),
+                         gfx::Tween::WINDRISE_EASE_OUT)))));
 }
