diff --git a/chrome/browser/ui/views/toolbar/avatar_toolbar_button_interface.cc b/chrome/browser/ui/views/toolbar/avatar_toolbar_button_interface.cc
index 0275722..4697a97 100644
--- a/chrome/browser/ui/views/toolbar/avatar_toolbar_button_interface.cc
+++ b/chrome/browser/ui/views/toolbar/avatar_toolbar_button_interface.cc
@@ -38,6 +38,16 @@ views::BubbleAnchor AvatarToolbarButtonInterface::GetBubbleAnchor(
     return views::BubbleAnchor(avatar_toolbar_button);
   }
 
+  // BrowserOS: the avatar can be hidden (e.g. by
+  // --disable-browseros-features=AvatarButton), and a hidden view is not a
+  // tracked element. Anchor its bubbles to the app menu button rather than to
+  // nothing; callers such as the incognito clear-data dialog assume an anchor.
+  if (!BrowserElements::From(&browser)->GetElement(
+          kToolbarAvatarButtonElementId)) {
+    return views::BubbleAnchor(BrowserElements::From(&browser)->GetElement(
+        kToolbarAppMenuButtonElementId));
+  }
+
   // Fallback to the Views AvatarToolbarButton if it is hidden (e.g. overflowed
   // into the toolbar overflow menu).
   if (auto* browser_view = BrowserView::GetBrowserViewForBrowser(&browser);
