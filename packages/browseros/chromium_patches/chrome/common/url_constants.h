diff --git a/chrome/common/url_constants.h b/chrome/common/url_constants.h
index da1d67be3d..125b0ed5d4 100644
--- a/chrome/common/url_constants.h
+++ b/chrome/common/url_constants.h
@@ -82,6 +82,12 @@ inline constexpr char kChooserUsbOverviewURL[] =
 inline constexpr char kChromeBetaForumURL[] =
     "https://support.google.com/chrome?p=beta_forum";
 
+// Shared by Windrise's About page, Help menu and keyboard shortcut.
+inline constexpr char kWindriseHelpChineseURL[] =
+    "https://www.windrisebridge.cn/help";
+inline constexpr char kWindriseHelpURL[] =
+    "https://www.windrisebridge.com/help";
+
 // General help links for Chrome, opened using various actions.
 inline constexpr char kChromeHelpViaKeyboardURL[] =
 #if BUILDFLAG(IS_CHROMEOS)
