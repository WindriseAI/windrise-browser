diff --git a/ui/views/bubble/bubble_dialog_delegate_view.cc b/ui/views/bubble/bubble_dialog_delegate_view.cc
index 53e90715ea..129587fbad 100644
--- a/ui/views/bubble/bubble_dialog_delegate_view.cc
+++ b/ui/views/bubble/bubble_dialog_delegate_view.cc
@@ -53,6 +53,7 @@
 #include "ui/views/accessibility/view_accessibility.h"
 #include "ui/views/background.h"
 #include "ui/views/bubble/bubble_frame_view.h"
+#include "base/i18n/rtl.h"
 #include "ui/views/bubble_histograms_variant.h"
 #include "ui/views/interaction/element_tracker_views.h"
 #include "ui/views/layout/layout_manager.h"
@@ -73,6 +74,10 @@
 #else
 #include "ui/aura/window.h"
 #include "ui/aura/window_observer.h"
+#include "ui/compositor/scoped_layer_animation_settings.h"
+#include "ui/gfx/animation/tween.h"
+#include "ui/gfx/geometry/transform_util.h"
+#include "ui/wm/core/window_animations.h"
 #endif
 
 #if BUILDFLAG(IS_OZONE)
@@ -180,6 +185,80 @@ class BubbleDialogFrameView : public BubbleFrameView {
 };
 
 // Create a widget to host the bubble.
+#if !BUILDFLAG(IS_MAC)
+// Windrise motion-1115 N3/N4: the shared bubble open / close animation, plugged
+// into aura's window visibility animation (VisibilityController, which
+// already respects the system "show animations" setting, and which
+// DesktopWindowTreeHostWin waits on before destroying the HWND on close).
+// Upstream default on Windows was DROP: opacity + scale 0.95 about the
+// center, 120ms LINEAR, both ways.
+constexpr base::TimeDelta kWindriseBubbleShow = base::Milliseconds(240);
+constexpr base::TimeDelta kWindriseBubbleHide = base::Milliseconds(180);
+constexpr float kWindriseBubbleStartScale = 0.96f;
+
+// The point the bubble grows from: the side its anchor is on (from the arrow;
+// no arrow -> center).
+gfx::Point WindriseBubblePivot(aura::Window* window) {
+  const gfx::Size size = window->bounds().size();
+  const gfx::Point center(size.width() / 2, size.height() / 2);
+  Widget* widget = Widget::GetWidgetForNativeWindow(window);
+  BubbleDialogDelegate* delegate =
+      widget && widget->widget_delegate()
+          ? widget->widget_delegate()->AsBubbleDialogDelegate()
+          : nullptr;
+  if (!delegate) {
+    return center;
+  }
+  BubbleBorder::Arrow arrow = delegate->arrow();
+  if (base::i18n::IsRTL()) {
+    arrow = BubbleBorder::horizontal_mirror(arrow);
+  }
+  if (!BubbleBorder::has_arrow(arrow)) {
+    return center;
+  }
+  const bool left = BubbleBorder::is_arrow_on_left(arrow);
+  const bool top = BubbleBorder::is_arrow_on_top(arrow);
+  const bool at_center = BubbleBorder::is_arrow_at_center(arrow);
+  if (BubbleBorder::is_arrow_on_horizontal(arrow)) {
+    return gfx::Point(at_center ? center.x() : (left ? 0 : size.width()),
+                      top ? 0 : size.height());
+  }
+  return gfx::Point(left ? 0 : size.width(),
+                    at_center ? center.y() : (top ? 0 : size.height()));
+}
+
+void AnimateWindriseBubbleVisibility(aura::Window* window, bool visible) {
+  ui::Layer* layer = window->layer();
+  if (visible) {
+    // Interrupted while closing: animate from the current on-screen values.
+    if (!layer->GetAnimator()->is_animating()) {
+      layer->SetOpacity(0.0f);
+      layer->SetTransform(gfx::GetScaleTransform(WindriseBubblePivot(window),
+                                                 kWindriseBubbleStartScale));
+    }
+    layer->SetVisible(true);
+    ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
+    settings.SetPreemptionStrategy(
+        ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
+    settings.SetTransitionDuration(kWindriseBubbleShow);
+    settings.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
+    layer->SetTransform(gfx::Transform());
+    layer->SetOpacity(1.0f);
+    return;
+  }
+  // Close: opacity only (no move), faster than opening.
+  wm::ScopedHidingAnimationSettings hiding(window);
+  ui::ScopedLayerAnimationSettings* settings =
+      hiding.layer_animation_settings();
+  settings->SetPreemptionStrategy(
+      ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
+  settings->SetTransitionDuration(kWindriseBubbleHide);
+  settings->SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
+  layer->SetOpacity(0.0f);
+  layer->SetVisible(false);
+}
+#endif  // !BUILDFLAG(IS_MAC)
+
 Widget* CreateBubbleWidget(BubbleDialogDelegate* bubble,
                            Widget::InitParams::Ownership ownership =
                                Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET) {
@@ -218,6 +297,19 @@ Widget* CreateBubbleWidget(BubbleDialogDelegate* bubble,
   bubble->OnBeforeBubbleWidgetInit(&bubble_params, bubble_widget.get());
   DCHECK(bubble_params.parent || !bubble->has_parent());
   bubble_widget->Init(std::move(bubble_params));
+#if !BUILDFLAG(IS_MAC)
+  // Windrise motion-1115 N3/N4. Bubbles that turned the visibility animation
+  // off themselves (SetVisibilityAnimationTransition(ANIMATE_NONE),
+  // kAnimationsDisabledKey) still get none.
+  if (!bubble->windrise_use_default_visibility_animation()) {
+    if (aura::Window* window = bubble_widget->GetNativeWindow()) {
+      wm::SetWindowVisibilityAnimationType(
+          window, wm::WINDOW_VISIBILITY_ANIMATION_TYPE_CUSTOM);
+      wm::SetWindowVisibilityCustomAnimation(
+          window, base::BindRepeating(&AnimateWindriseBubbleVisibility));
+    }
+  }
+#endif
   return bubble_widget.release();
 }
 
@@ -484,6 +576,11 @@ BubbleDialogDelegate::BubbleDialogDelegate(BubbleAnchor anchor,
   SetShowCloseButton(false);
 
   LayoutProvider* const layout_provider = LayoutProvider::Get();
+  // 风桥 N2：气泡圆角独立于对话框。对话框走 kDialogRadius（ChromeLayoutProvider
+  // 把它映射到 radius_large = 16），气泡在这里固定成菜单 / 气泡档
+  // （radius_medium = 12）。派生类里显式 set_corner_radius 的仍然覆盖这里。
+  set_corner_radius(
+      layout_provider->GetCornerRadiusMetric(views::kMenuRadius));
   set_frame_margins(
       {.contents = layout_provider->GetDialogInsetsForContentType(
            DialogContentType::kText, DialogContentType::kText),
