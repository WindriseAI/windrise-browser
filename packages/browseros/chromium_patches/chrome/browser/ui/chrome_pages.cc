diff --git a/chrome/browser/ui/chrome_pages.cc b/chrome/browser/ui/chrome_pages.cc
index 9bc0bdcaff..b4a9595db1 100644
--- a/chrome/browser/ui/chrome_pages.cc
+++ b/chrome/browser/ui/chrome_pages.cc
@@ -23,6 +23,7 @@
 #include "chrome/browser/apps/app_service/app_service_proxy.h"
 #include "chrome/browser/apps/app_service/app_service_proxy_factory.h"
 #include "chrome/browser/bookmarks/bookmark_model_factory.h"
+#include "chrome/browser/browser_process.h"
 #include "chrome/browser/download/bubble/download_bubble_ui_controller.h"
 #include "chrome/browser/extensions/extension_util.h"
 #include "chrome/browser/file_system_access/file_system_access_features.h"
@@ -170,25 +171,22 @@ void ShowHelpImpl(BrowserWindowInterface* browser,
   GURL url;
   switch (source) {
     case HelpSource::kKeyboard:
-      url = GURL(kChromeHelpViaKeyboardURL);
-      break;
     case HelpSource::kMenu:
-      url = GURL(kChromeHelpViaMenuURL);
+    case HelpSource::kWebUI: {
+      const std::string& locale = g_browser_process->GetApplicationLocale();
+      const bool is_chinese =
+          base::EqualsCaseInsensitiveASCII(locale, "zh") ||
+          base::StartsWith(locale, "zh-", base::CompareCase::INSENSITIVE_ASCII);
+      url = GURL(is_chinese ? kWindriseHelpChineseURL : kWindriseHelpURL);
       break;
+    }
     case HelpSource::kWebHID:
       url = GURL(kChooserHidOverviewUrl);
       break;
 #if BUILDFLAG(IS_CHROMEOS)
-    case HelpSource::kWebUI:
-      url = GURL(kChromeHelpViaWebUIURL);
-      break;
     case HelpSource::kWebUIChromeOS:
       url = GURL(kChromeOsHelpViaWebUIURL);
       break;
-#else
-    case HelpSource::kWebUI:
-      url = GURL(kChromeHelpViaWebUIURL);
-      break;
 #endif  // BUILDFLAG(IS_CHROMEOS)
     case HelpSource::kWebUSD:
       url = GURL(kChooserUsbOverviewURL);
