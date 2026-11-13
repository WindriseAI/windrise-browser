diff --git a/chrome/browser/extensions/chrome_extension_system.cc b/chrome/browser/extensions/chrome_extension_system.cc
index 53e2ad0e8..b2e2ed3ca 100644
--- a/chrome/browser/extensions/chrome_extension_system.cc
+++ b/chrome/browser/extensions/chrome_extension_system.cc
@@ -18,9 +18,11 @@
 #include "build/build_config.h"
 #include "build/chromeos_buildflags.h"
 #include "chrome/browser/browser_process.h"
+#include "chrome/browser/browseros/extensions/browseros_extension_loader.h"
 #include "chrome/browser/extensions/blocklist_factory.h"
 #include "chrome/browser/extensions/chrome_content_verifier_delegate.h"
 #include "chrome/browser/extensions/chrome_extension_system_factory.h"
+#include "chrome/browser/extensions/windrise_extension_protection.h"
 #include "chrome/browser/extensions/component_loader.h"
 #include "chrome/browser/extensions/extension_error_controller.h"
 #include "chrome/browser/extensions/extension_garbage_collector.h"
@@ -64,6 +66,7 @@
 #include "extensions/browser/user_script_manager.h"
 #include "extensions/buildflags/buildflags.h"
 #include "extensions/common/constants.h"
+#include "extensions/common/extension.h"
 #include "extensions/common/features/feature_channel.h"
 #include "extensions/common/manifest_handlers/manifest_url_handlers.h"
 #include "ui/message_center/public/cpp/notifier_id.h"
@@ -96,6 +99,37 @@ namespace extensions {
 
 namespace {
 
+// Keep product policy in the Chrome layer and leave the generic idle gate
+// unchanged. Both downloaded installs and startup-delayed installs cross this
+// seam; checking before the base gate prevents its immediate/startup exemptions
+// from replacing a working fresh bundle during the persisted grace period.
+class BrowserOSUpdateInstallGate : public UpdateInstallGate {
+ public:
+  explicit BrowserOSUpdateInstallGate(Profile* profile)
+      : UpdateInstallGate(profile), profile_(profile) {}
+
+  Action ShouldDelay(const Extension* extension,
+                     bool install_immediately) override {
+    using Policy = browseros::BrowserOSExtensionLoader::UpdateInstallPolicy;
+    switch (browseros::BrowserOSExtensionLoader::GetUpdateInstallPolicy(
+        profile_, extension->id())) {
+      case Policy::kDefer:
+        return DELAY;
+      case Policy::kInstallImmediately:
+        // The download may finish after expiry's one-time release attempt.
+        // Override only idleness; the manager still runs its other gates.
+        install_immediately = true;
+        break;
+      case Policy::kNormal:
+        break;
+    }
+    return UpdateInstallGate::ShouldDelay(extension, install_immediately);
+  }
+
+ private:
+  const raw_ptr<Profile> profile_;
+};
+
 // Helper to serve as an UninstallPingSender::Filter callback.
 UninstallPingSender::FilterResult ShouldSendUninstallPing(
     Profile* profile,
@@ -156,6 +190,7 @@ void ChromeExtensionSystem::Shared::InitPrefs() {
 }
 
 void ChromeExtensionSystem::Shared::RegisterManagementPolicyProviders() {
+  RegisterWindriseExtensionProtection(management_policy_.get());
   management_policy_->RegisterProviders(
       ExtensionManagementFactory::GetForBrowserContext(profile_)
           ->GetProviders());
@@ -194,7 +229,7 @@ void ChromeExtensionSystem::Shared::RegisterManagementPolicyProviders() {
 }
 
 void ChromeExtensionSystem::Shared::InitInstallGates() {
-  update_install_gate_ = std::make_unique<UpdateInstallGate>(profile_);
+  update_install_gate_ = std::make_unique<BrowserOSUpdateInstallGate>(profile_);
   auto* delayed_install_manager = DelayedInstallManager::Get(profile_);
   delayed_install_manager->RegisterInstallGate(
       ExtensionPrefs::DelayReason::kWaitForIdle, update_install_gate_.get());
