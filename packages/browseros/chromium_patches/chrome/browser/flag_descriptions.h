diff --git a/chrome/browser/flag_descriptions.h b/chrome/browser/flag_descriptions.h
index 9d1ff04..c0a3d51 100644
--- a/chrome/browser/flag_descriptions.h
+++ b/chrome/browser/flag_descriptions.h
@@ -353,6 +353,18 @@ inline constexpr char kBlockingFocusWithoutUserActivationDescription[] =
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
 inline constexpr char kBrowsingHistoryImprovedHostnameSuffixMatchingName[] =
     "Browsing History Improved Hostname Suffix Matching";
 inline constexpr char
