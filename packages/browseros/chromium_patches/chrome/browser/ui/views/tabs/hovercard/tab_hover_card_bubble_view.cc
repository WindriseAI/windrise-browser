diff --git a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
index e582c70239..ffd689a854 100644
--- a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
+++ b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
@@ -664,6 +664,9 @@ TabHoverCardBubbleView::TabHoverCardBubbleView(
   SetCanActivate(false);
 #if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
   set_accept_events(false);
+  // Windrise motion-1115: the hover card runs its own WidgetFadeAnimator;
+  // keep upstream's default visibility animation (decision d: unchanged).
+  set_windrise_use_default_visibility_animation(true);
 #endif
   set_accept_drag_drop(false);
 
