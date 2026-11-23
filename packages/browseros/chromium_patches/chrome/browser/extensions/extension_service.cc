diff --git a/chrome/browser/extensions/extension_service.cc b/chrome/browser/extensions/extension_service.cc
--- a/chrome/browser/extensions/extension_service.cc
+++ b/chrome/browser/extensions/extension_service.cc
@@ -38,6 +38,8 @@
 #include "build/build_config.h"
 #include "build/chromeos_buildflags.h"
 #include "chrome/browser/browser_process.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
+#include "chrome/browser/browseros/extensions/browseros_extension_loader.h"
 #include "chrome/browser/extensions/chrome_extension_registrar_delegate.h"
 #include "chrome/browser/extensions/component_loader.h"
 #include "chrome/browser/extensions/corrupted_extension_reinstaller.h"
@@ -372,7 +374,10 @@

   LoadExtensionsFromCommandLineFlag(switches::kDisableExtensionsExcept);
   if (load_command_line_extensions) {
-    LoadExtensionsFromCommandLineFlag(switches::kLoadExtension);
+    const bool desktop_assistant =
+        LoadExtensionsFromCommandLineFlag(switches::kLoadExtension);
+    browseros::BrowserOSExtensionLoader::LoadBundledWindriseAssistantIfNeeded(
+        profile_, desktop_assistant);
   }
   EnabledReloadableExtensions();
   delayed_install_manager_->FinishInstallationsDelayedByShutdown();
@@ -406,12 +411,12 @@
   extension_registrar_->EnabledReloadableExtensions();
 }

-void ExtensionService::LoadExtensionsFromCommandLineFlag(
+bool ExtensionService::LoadExtensionsFromCommandLineFlag(
     const char* switch_name) {
   CHECK(switch_name == switches::kLoadExtension ||
         switch_name == switches::kDisableExtensionsExcept);
   if (!command_line_->HasSwitch(switch_name)) {
-    return;
+    return false;
   }

   // Check that --load-extension is allowed.
@@ -419,12 +424,12 @@
 #if BUILDFLAG(GOOGLE_CHROME_BRANDING) && !BUILDFLAG(IS_CHROMEOS)
     LOG(WARNING)
         << "--load-extension is not allowed in Google Chrome, ignoring.";
-    return;
+    return false;
 #else   // BUILDFLAG(GOOGLE_CHROME_BRANDING) && !BUILDFLAG(IS_CHROMEOS)
     if (safe_browsing::IsEnhancedProtectionEnabled(*profile_->GetPrefs())) {
       VLOG(1) << "--load-extension is not allowed for users opted into "
               << "Enhanced Safe Browsing, ignoring.";
-      return;
+      return false;
     }
     if (ShouldBlockCommandLineExtension(*profile_)) {
       // TODO(crbug.com/401529219): Deprecate this restriction once
@@ -432,7 +437,7 @@
       VLOG(1)
           << "--load-extension is not allowed for users that have the policy "
           << "ExtensionInstallTypeBlocklist::command_line, ignoring.";
-      return;
+      return false;
     }
 #endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING) && !BUILDFLAG(IS_CHROMEOS)
   } else if (base::FeatureList::IsEnabled(
@@ -442,7 +447,7 @@
              base::NotFatalUntil::M161);
     LOG(WARNING) << "--disable-extensions-except is not allowed in Google "
                     "Chrome, ignoring.";
-    return;
+    return false;
   }

   base::CommandLine::StringType path_list =
@@ -450,11 +455,17 @@
   base::StringTokenizerT<base::CommandLine::StringType,
                          base::CommandLine::StringType::const_iterator>
       t(path_list, FILE_PATH_LITERAL(","));
+  bool found_signed_assistant = false;
   while (t.GetNext()) {
     std::string extension_id;
+    const base::FilePath path(t.token_piece());
     UnpackedInstaller::Create(profile_)->LoadFromCommandLine(
-        base::FilePath(t.token_piece()), &extension_id,
-        /*only-allow-apps=*/false);
+        path, &extension_id, /*only_allow_apps=*/false);
+    if (switch_name == switches::kLoadExtension &&
+        extension_id == browseros::kWindriseAssistantExtensionId) {
+      found_signed_assistant |=
+          browseros::BrowserOSExtensionLoader::IsSignedWindriseAssistantPath(path);
+    }
     if (switch_name == switches::kDisableExtensionsExcept) {
       extension_registrar_->AddDisableFlagExemptedExtension(extension_id);
     }
@@ -465,6 +476,7 @@
       switch_name == switches::kLoadExtension
           ? ExtensionService::LoadExtensionFlag::kLoadExtension
           : ExtensionService::LoadExtensionFlag::kDisableExtensionsExcept);
+  return found_signed_assistant;
 }

 #if BUILDFLAG(IS_CHROMEOS)
