diff --git a/ui/views/controls/menu/menu_host.cc b/ui/views/controls/menu/menu_host.cc
index 293bacaa70..d3a2deb863 100644
--- a/ui/views/controls/menu/menu_host.cc
+++ b/ui/views/controls/menu/menu_host.cc
@@ -37,6 +37,10 @@
 #if defined(USE_AURA)
 #include "ui/aura/client/aura_constants.h"
 #include "ui/aura/window.h"
+#include "ui/compositor/layer.h"
+#include "ui/compositor/scoped_layer_animation_settings.h"
+#include "ui/gfx/animation/tween.h"
+#include "ui/wm/core/window_animations.h"
 #endif
 
 #if BUILDFLAG(IS_OZONE)
@@ -117,6 +121,33 @@ void TransferGesture(ui::GestureRecognizer* gesture_recognizer,
 
 }  // namespace internal
 
+#if defined(USE_AURA)
+namespace {
+
+// Windrise mouse-opened root menus
+// fade in over 120ms (press step) on gfx::Tween::WINDRISE_EASE_OUT, opacity
+// only; no closing animation; keyboard-opened menus and submenus appear
+// instantly. Upstream default on Windows was FADE 150ms LINEAR both ways.
+void AnimateWindriseMenuVisibility(aura::Window* window, bool visible) {
+  ui::Layer* layer = window->layer();
+  if (!visible) {
+    // Only reached if a hide transition was configured; hide at once.
+    layer->SetVisible(false);
+    return;
+  }
+  layer->SetOpacity(0.0f);
+  layer->SetVisible(true);
+  ui::ScopedLayerAnimationSettings settings(layer->GetAnimator());
+  settings.SetPreemptionStrategy(
+      ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
+  settings.SetTransitionDuration(base::Milliseconds(120));
+  settings.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
+  layer->SetOpacity(1.0f);
+}
+
+}  // namespace
+#endif  // USE_AURA
+
 ////////////////////////////////////////////////////////////////////////////////
 // MenuHost, public:
 
@@ -166,6 +197,22 @@ void MenuHost::InitMenuHost(const InitParams& init_params) {
   }
 
   Init(std::move(params));
+#if defined(USE_AURA)
+  // Windrise .
+  if (aura::Window* window = GetNativeWindow()) {
+    const bool is_root_menu = !submenu_->GetMenuItem()->GetParentMenuItem();
+    if (is_root_menu && menu_controller &&
+        menu_controller->windrise_opened_by_mouse()) {
+      wm::SetWindowVisibilityAnimationType(
+          window, wm::WINDOW_VISIBILITY_ANIMATION_TYPE_CUSTOM);
+      wm::SetWindowVisibilityCustomAnimation(
+          window, base::BindRepeating(&AnimateWindriseMenuVisibility));
+      wm::SetWindowVisibilityAnimationTransition(window, wm::ANIMATE_SHOW);
+    } else {
+      wm::SetWindowVisibilityAnimationTransition(window, wm::ANIMATE_NONE);
+    }
+  }
+#endif
   std::optional<std::string> show_menu_host_duration_histogram =
       menu_controller->TakeShowMenuHostDurationHistogram();
   CHECK(!menu_controller->TakeShowMenuHostDurationHistogram().has_value());
