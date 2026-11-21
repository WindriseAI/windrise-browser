diff --git a/chrome/browser/chrome_browser_main.cc b/chrome/browser/chrome_browser_main.cc
index 9aa6f338c73e80ee878c068db1811230ee3e6339..2393744d830c67d80d55cf0da6c0aac752b4c1ed 100644
--- a/chrome/browser/chrome_browser_main.cc
+++ b/chrome/browser/chrome_browser_main.cc
@@ -34,6 +34,8 @@
 #include "chrome/browser/browser_features.h"
 #include "chrome/browser/browser_process.h"
 #include "chrome/browser/browser_process_impl.h"
+#include "chrome/browser/browseros/server/browseros_server_manager.h"
+#include "chrome/browser/browseros/server/browseros_server_prefs.h"
 #include "chrome/browser/chrome_browser_main_extra_parts.h"
 #include "chrome/browser/component_updater/registration.h"
 #include "chrome/browser/enterprise/browser_management/management_service_factory.h"
@@ -1005,6 +1007,11 @@ void ChromeBrowserMainParts::RecordBrowserStartupTime() {
 
 int ChromeBrowserMainParts::PreEarlyInitialization() {
   TRACE_EVENT0("startup", "ChromeBrowserMainParts::PreEarlyInitialization");
+  // Windrise distributes component revisions with the browser package.
+  // Set Chromium's existing switch before any component service is created;
+  // it disables scheduled and on-demand updates, including policy exemptions.
+  base::CommandLine::ForCurrentProcess()->AppendSwitch(
+      switches::kDisableComponentUpdate);
   for (auto& chrome_extra_part : chrome_extra_parts_) {
     chrome_extra_part->PreEarlyInitialization();
   }
@@ -1359,6 +1366,43 @@ int ChromeBrowserMainParts::PreCreateThreadsImpl() {
   }
 #endif
 
+#if BUILDFLAG(IS_MAC)
+  // Install iCloud Passwords native messaging host manifest.
+  //
+  // Why this runs on every startup (not just first run):
+  // - First-run only would miss existing users upgrading to this version
+  // - The "First Run" sentinel already exists for them, so IsChromeFirstRun()
+  //   returns false and first-run code is skipped entirely
+  // - Running every startup also self-heals if the manifest is deleted
+  // - The PathExists check makes this cheap (~0.1ms) when file already exists
+  {
+    base::FilePath native_messaging_dir;
+    if (base::PathService::Get(chrome::DIR_USER_NATIVE_MESSAGING,
+                               &native_messaging_dir)) {
+      // Ensure directory exists for users who installed before first-run
+      // directory creation was added.
+      if (!base::PathExists(native_messaging_dir))
+        base::CreateDirectory(native_messaging_dir);
+
+      const base::FilePath manifest_path =
+          native_messaging_dir.Append("com.apple.passwordmanager.json");
+      if (!base::PathExists(manifest_path)) {
+        constexpr std::string_view kICloudPasswordsManifest = R"({
+    "name": "com.apple.passwordmanager",
+    "description": "PasswordManagerBrowserExtensionHelper",
+    "path": "/System/Cryptexes/App/System/Library/CoreServices/PasswordManagerBrowserExtensionHelper.app/Contents/MacOS/PasswordManagerBrowserExtensionHelper",
+    "type": "stdio",
+    "allowed_origins": [
+        "chrome-extension://pejdijmoenmkgeppbflobdenhhabjlaj/",
+        "chrome-extension://mfbcdcnpokpoajjciilocoachedjkima/"
+    ]
+})";
+        base::WriteFile(manifest_path, kICloudPasswordsManifest);
+      }
+    }
+  }
+#endif  // BUILDFLAG(IS_MAC)
+
 #if BUILDFLAG(IS_MAC)
 #if defined(ARCH_CPU_X86_64)
   // The use of Rosetta to run the x64 version of Chromium on Arm is neither
@@ -1956,12 +2000,20 @@ int ChromeBrowserMainParts::PreMainMessageLoopRunImpl() {
     g_browser_process->CreateDevToolsAutoOpener();
   }
 
-  // Needs to be done before PostProfileInit, since the SODA Installer setup is
-  // called inside PostProfileInit and depends on it.
-  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
-          switches::kDisableComponentUpdate)) {
-    component_updater::RegisterComponentsForUpdate();
-  }
+  // Migrate legacy browseros.server.* local state prefs to windrise.server.*.
+  if (PrefService* local_state = g_browser_process->local_state()) {
+    browseros_server::MigrateLocalStatePrefs(local_state);
+  }
+
+  // Explicit debugging transports and --disable-browseros-server take
+  // precedence over the managed service's CDP listener.
+  LOG(INFO) << "windrise: Starting Windrise server process";
+  browseros::BrowserOSServerManager::GetInstance()->Start();
+
+  // Register packaged/cached components before PostProfileInit (including SODA).
+  // The native disable-component-update default stops network checks in the
+  // component service while allowing installers to load local revisions.
+  component_updater::RegisterComponentsForUpdate();
 
   // `profile` may be nullptr if the profile picker is shown.
   Profile* profile = profile_info.profile;
@@ -2261,6 +2313,10 @@ void ChromeBrowserMainParts::PostMainMessageLoopRun() {
     chrome_extra_part->PostMainMessageLoopRun();
   }
 
+  // BrowserOS: Stop the BrowserOS server during shutdown
+  LOG(INFO) << "windrise: Stopping Windrise server process";
+  browseros::BrowserOSServerManager::GetInstance()->Shutdown();
+
   TranslateService::Shutdown();
 
 #if BUILDFLAG(ENABLE_PROCESS_SINGLETON)
