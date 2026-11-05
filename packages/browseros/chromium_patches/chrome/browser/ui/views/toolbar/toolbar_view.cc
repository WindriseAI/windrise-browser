diff --git a/chrome/browser/ui/views/toolbar/toolbar_view.cc b/chrome/browser/ui/views/toolbar/toolbar_view.cc
index 7f404c6..c43d5f2 100644
--- a/chrome/browser/ui/views/toolbar/toolbar_view.cc
+++ b/chrome/browser/ui/views/toolbar/toolbar_view.cc
@@ -23,6 +23,9 @@
 #include "build/build_config.h"
 #include "chrome/app/chrome_command_ids.h"
 #include "chrome/browser/actor/ui/actor_ui_metrics.h"
+#include "chrome/browser/actor/ui/task_list_bubble/actor_task_list_bubble.h"
+#include "chrome/browser/actor/ui/task_list_bubble/actor_task_list_bubble_controller.h"
+#include "chrome/browser/browseros/customization/customization_pref_names.h"
 #include "chrome/browser/command_updater.h"
 #include "chrome/browser/glic/browser_ui/activity_list_bubble/glic_activity_list_bubble.h"
 #include "chrome/browser/glic/browser_ui/activity_list_bubble/glic_activity_list_bubble_controller.h"
@@ -416,7 +419,14 @@ void ToolbarView::Init() {
   // profile (only regular and incognito profiles host extensions).
   if (!browser_->GetProfile()->IsGuestSession() &&
       !features::IsWebUIExtensionsContainerEnabled()) {
-    extensions_container = std::make_unique<ExtensionsToolbarDesktop>(browser_);
+    // BrowserOS: with browseros.show_extensions_toolbar off, e.g. forced by
+    // --disable-browseros-features=ExtensionsToolbar, use the auto-hide mode of
+    // web-app windows. The area then appears only while an extension popup or
+    // the extensions menu needs it, so popups keep their anchor.
+    extensions_container = std::make_unique<ExtensionsToolbarDesktop>(
+        browser_, prefs->GetBoolean(browseros::prefs::kShowExtensionsToolbar)
+                      ? ExtensionsToolbarDesktop::DisplayMode::kNormal
+                      : ExtensionsToolbarDesktop::DisplayMode::kAutoHide);
 
     toolbar_divider = std::make_unique<ToolbarDivider>();
   }
@@ -607,6 +617,12 @@ void ToolbarView::Init() {
     avatar_ = AddChildView(std::make_unique<AvatarToolbarButton>(browser_));
     bool show_avatar_toolbar_button =
         AvatarToolbarButtonInterface::CanShowForProfile(browser_->GetProfile());
+    // BrowserOS: browseros.show_avatar_button can hide it, e.g. forced off by
+    // --disable-browseros-features=AvatarButton. Nothing changes the avatar's
+    // visibility after this, so reading the pref once per window is enough.
+    if (!prefs->GetBoolean(browseros::prefs::kShowAvatarButton)) {
+      show_avatar_toolbar_button = false;
+    }
     avatar_->SetVisible(show_avatar_toolbar_button);
   }
 
