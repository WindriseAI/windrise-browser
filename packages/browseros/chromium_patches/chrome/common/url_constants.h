diff --git a/chrome/common/url_constants.h b/chrome/common/url_constants.h
index da1d67be3d..5b5e0cdc5c 100644
--- a/chrome/common/url_constants.h
+++ b/chrome/common/url_constants.h
@@ -82,6 +82,24 @@ inline constexpr char kChooserUsbOverviewURL[] =
 inline constexpr char kChromeBetaForumURL[] =
     "https://support.google.com/chrome?p=beta_forum";
 
+// Shared by Windrise's About page, Help menu and keyboard shortcut.
+inline constexpr char kWindriseHelpChineseURL[] =
+    "https://www.windrisebridge.cn/help";
+inline constexpr char kWindriseHelpURL[] =
+    "https://www.windrisebridge.com/help";
+
+// Windrise legal pages, shown from the About page. The Chinese site serves the
+// zh UI, the English site every other UI language. Values mirror the two site
+// footers (about page rows read them through the settings WebUI data source).
+inline constexpr char kWindrisePrivacyChineseURL[] =
+    "https://www.windrisebridge.cn/privacy";
+inline constexpr char kWindriseTermsChineseURL[] =
+    "https://www.windrisebridge.cn/terms";
+inline constexpr char kWindrisePrivacyURL[] =
+    "https://www.windrisebridge.com/privacy";
+inline constexpr char kWindriseTermsURL[] =
+    "https://www.windrisebridge.com/terms";
+
 // General help links for Chrome, opened using various actions.
 inline constexpr char kChromeHelpViaKeyboardURL[] =
 #if BUILDFLAG(IS_CHROMEOS)
