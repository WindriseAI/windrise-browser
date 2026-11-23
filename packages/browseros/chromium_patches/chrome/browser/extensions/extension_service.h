diff --git a/chrome/browser/extensions/extension_service.h b/chrome/browser/extensions/extension_service.h
--- a/chrome/browser/extensions/extension_service.h
+++ b/chrome/browser/extensions/extension_service.h
@@ -258,7 +258,8 @@

  private:
   // Loads extensions specified via a command line flag/switch.
-  void LoadExtensionsFromCommandLineFlag(const char* switch_name);
+  // Returns whether --load-extension submitted a signed Windrise assistant.
+  bool LoadExtensionsFromCommandLineFlag(const char* switch_name);
 #if BUILDFLAG(IS_CHROMEOS)
   void LoadSigninProfileTestExtension(const std::string& path);
 #endif
