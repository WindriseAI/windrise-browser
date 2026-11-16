diff --git a/chrome/browser/ui/webui/current_channel_logo.cc b/chrome/browser/ui/webui/current_channel_logo.cc
index 70f983e5c6..34525ecfc5 100644
--- a/chrome/browser/ui/webui/current_channel_logo.cc
+++ b/chrome/browser/ui/webui/current_channel_logo.cc
@@ -33,7 +33,15 @@ int CurrentChannelLogoResourceId() {
       [[fallthrough]];
 #endif
     case version_info::Channel::UNKNOWN:
+#if !BUILDFLAG(GOOGLE_CHROME_BRANDING)
+      // Windrise 甲40 / 甲41（用户 11-13 R522「关于页……logo 还没换过来」）：
+      // 网页里代表「这个浏览器」的标志用 B6 不带底的墨色图形标（关于页卡片、
+      // 设置 / 历史 / 下载页头，效果图 mock/content-about-zh.png）。
+      // IDR_PRODUCT_LOGO_32 本身仍是带乳白玻璃底的程序图标，原生界面照旧用它。
+      return IDR_WINDRISE_BROWSER_MARK_32;
+#else
       return IDR_PRODUCT_LOGO_32;
+#endif
   }
   return -1;
 }
