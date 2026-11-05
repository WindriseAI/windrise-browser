diff --git a/chrome/common/chrome_paths.h b/chrome/common/chrome_paths.h
index abeb4f2..56a065e 100644
--- a/chrome/common/chrome_paths.h
+++ b/chrome/common/chrome_paths.h
@@ -56,10 +56,10 @@ enum {
                      // to set policies for chrome. This directory
                      // contains subdirectories.
 #endif
-#if BUILDFLAG(IS_CHROMEOS) || \
+#if BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_WIN) || \
     (BUILDFLAG(IS_LINUX) && BUILDFLAG(CHROMIUM_BRANDING)) || BUILDFLAG(IS_MAC)
   DIR_USER_EXTERNAL_EXTENSIONS,  // Directory for per-user external extensions
-                                 // on Chrome Mac and Chromium Linux.
+                                 // on Chrome Mac, Windows and Chromium Linux.
                                  // On Chrome OS, this path is used for OEM
                                  // customization. Getting this path does not
                                  // create it.
@@ -133,6 +133,9 @@ enum {
                           // dynamic patching is enabled; not available in
                           // other processes.
 #endif
+  DIR_BROWSEROS_BUNDLED_EXTENSIONS,  // Directory containing bundled BrowserOS
+                                     // extension CRX files for immediate
+                                     // installation on first run.
   PATH_END
 };
 
