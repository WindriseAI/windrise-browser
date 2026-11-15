diff --git a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
index e582c70239..d5d25fa958 100644
--- a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
+++ b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
@@ -665,6 +665,10 @@ TabHoverCardBubbleView::TabHoverCardBubbleView(
 #if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
   set_accept_events(false);
 #endif
+  // Windrise motion-1115: the hover card runs its own WidgetFadeAnimator;
+  // keep upstream's default visibility animation on every platform,
+  // Windows included (decision d: unchanged).
+  set_windrise_use_default_visibility_animation(true);
   set_accept_drag_drop(false);
 
   // Set so that the tab hover card is not focus traversable when keyboard
