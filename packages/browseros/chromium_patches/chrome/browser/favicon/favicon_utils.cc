diff --git a/chrome/browser/favicon/favicon_utils.cc b/chrome/browser/favicon/favicon_utils.cc
index 398c608..eb866e2 100644
--- a/chrome/browser/favicon/favicon_utils.cc
+++ b/chrome/browser/favicon/favicon_utils.cc
@@ -181,10 +181,9 @@ bool ShouldThemifyFaviconForEntry(content::NavigationEntry* entry) {
     return true;
   }
 
-  // Themify favicon for the default NTP and incognito NTP.
+  // Themify favicon for incognito NTP only; regular NTP uses the B6 logo.
   if (actual_url.SchemeIs(content::kChromeUIScheme)) {
-    return actual_url.host() == chrome::kChromeUINewTabPageHost ||
-           actual_url.host() == chrome::kChromeUINewTabHost;
+    return actual_url.host() == chrome::kChromeUINewTabHost;
   }
 
   return false;
