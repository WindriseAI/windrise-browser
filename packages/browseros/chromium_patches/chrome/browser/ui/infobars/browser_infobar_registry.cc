diff --git a/chrome/browser/ui/infobars/browser_infobar_registry.cc b/chrome/browser/ui/infobars/browser_infobar_registry.cc
index f581cb7..957d8eb 100644
--- a/chrome/browser/ui/infobars/browser_infobar_registry.cc
+++ b/chrome/browser/ui/infobars/browser_infobar_registry.cc
@@ -58,12 +58,12 @@
 
 #if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
 #include "chrome/browser/ui/views/session_restore_infobar/session_restore_infobar_manager.h"
+#include "chrome/grit/theme_resources.h"
 #endif
 
 #if BUILDFLAG(IS_MAC)
 #include "chrome/browser/updater/updater.h"
 #include "chrome/common/pref_names.h"
-#include "chrome/grit/theme_resources.h"
 #endif
 
 namespace infobars {
@@ -427,10 +427,12 @@ void RegisterInfoBars() {
             .SetLinkNavigationUrl(GURL("chrome://settings/onStartup"))
             .SetShouldShowLinkBeforeButton(true)
             .SetLinkSpacingWhenPositionedBeforeButton(4)
-            .SetIcon(vector_icons::kProductRefreshIcon)
-            .SetDarkModeIcon(features::IsRoundedIconsEnabled()
-                                 ? omnibox::kChromeProductIcon
-                                 : omnibox::kProductChromeRefreshOldIcon)
+            // This path bypasses SessionRestoreInfoBarDelegate, so select
+            // the same BrowserOS bitmap explicitly for both color modes.
+            // 158 keeps SetIcon()/SetDarkModeIcon() in this spec, but the
+            // vector icon takes precedence over SetIconId(), so the old
+            // Chrome product icons are intentionally left out here.
+            .SetIconId(IDR_PRODUCT_LOGO_16)
             .SetScope(InfoBarScope::kGlobal)
             .SetExpireOnNavigation(false)
             .SetBrowserFilter(
