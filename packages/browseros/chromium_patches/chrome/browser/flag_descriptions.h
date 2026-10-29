diff --git a/chrome/browser/flag_descriptions.h b/chrome/browser/flag_descriptions.h
index 9cacf518b077321fbd221174474fe1f7bc69be87..43c11d67bf6bfd1e1a27845c09fc86ac6a1bf16e 100644
--- a/chrome/browser/flag_descriptions.h
+++ b/chrome/browser/flag_descriptions.h
@@ -325,6 +325,18 @@ inline constexpr char kBlockingFocusWithoutUserActivationDescription[] =
     "(element.focus(), window.focus(), autofocus) from iframes unless "
     "triggered by a user gesture.";
 
+// BrowserOS: feature flags
+inline constexpr char kBrowserOsAlphaFeaturesName[] =
+    "Windrise Alpha Features";
+inline constexpr char kBrowserOsAlphaFeaturesDescription[] =
+    "Enables Windrise alpha features.";
+
+inline constexpr char kBrowserOsKeyboardShortcutsName[] =
+    "Windrise Keyboard Shortcuts";
+inline constexpr char kBrowserOsKeyboardShortcutsDescription[] =
+    "Enables Windrise keyboard shortcuts (Cmd+Shift+K, Cmd+Shift+L, "
+    "Option+A). Disable if these conflict with your keyboard layout.";
+
 inline constexpr char kBrowsingHistoryActorIntegrationM3Name[] =
     "Browsing History Actor Integration M3";
 inline constexpr char kBrowsingHistoryActorIntegrationM3Description[] =
