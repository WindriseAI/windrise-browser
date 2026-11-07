diff --git a/chrome/browser/devtools/protocol/target_handler.cc b/chrome/browser/devtools/protocol/target_handler.cc
index a70077e..7ede1f9 100644
--- a/chrome/browser/devtools/protocol/target_handler.cc
+++ b/chrome/browser/devtools/protocol/target_handler.cc
@@ -130,6 +130,17 @@ protocol::Response TargetHandler::CreateTarget(
     }
   }
 
+  // Explicit regular contexts are now discoverable without opening a window
+  // through the profile picker. Enforce its lock/policy checks here as well as
+  // in Browser.createWindow; Navigate's creation-status check omits the lock.
+  if (browser_context_id.has_value() && profile->IsRegularProfile() &&
+      !profile->IsOffTheRecord() &&
+      !DevToolsBrowserContextManager::GetInstance()
+           .CanCreateRegularProfileWindow(profile)) {
+    return protocol::Response::ServerError(
+        "Profile policy does not allow a regular window");
+  }
+
   bool create_new_window = new_window.value_or(false);
   const bool should_focus = focus.value_or(!background.value_or(false));
   const bool create_in_background =
