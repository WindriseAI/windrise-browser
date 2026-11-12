diff --git a/chrome/browser/ui/color/chrome_color_mixers.cc b/chrome/browser/ui/color/chrome_color_mixers.cc
index 8432f49468..c4f51f4bb4 100644
--- a/chrome/browser/ui/color/chrome_color_mixers.cc
+++ b/chrome/browser/ui/color/chrome_color_mixers.cc
@@ -23,6 +23,7 @@
 #include "chrome/browser/ui/color/omnibox_color_mixer.h"
 #include "chrome/browser/ui/color/product_specifications_color_mixer.h"
 #include "chrome/browser/ui/color/tab_strip_color_mixer.h"
+#include "chrome/browser/ui/color/windrise_color_mixer.h"
 #include "ui/color/color_provider_utils.h"
 
 namespace {
@@ -95,4 +96,8 @@ void AddChromeColorMixers(ui::ColorProvider* provider,
   if (key.app_controller) {
     key.app_controller->AddColorMixers(provider, key);
   }
+
+  // 风桥整窗配色，最后运行以覆盖上面的取值。读 browseros.windrise.theme.*
+  // 偏好；偏好变了由 ThemeService 清颜色缓存并通知重绘。
+  AddWindriseColorMixer(provider, key);
 }
