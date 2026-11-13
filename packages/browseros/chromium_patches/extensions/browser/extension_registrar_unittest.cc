diff --git a/extensions/browser/extension_registrar_unittest.cc b/extensions/browser/extension_registrar_unittest.cc
index f7415f2a4..6700bc5b2 100644
--- a/extensions/browser/extension_registrar_unittest.cc
+++ b/extensions/browser/extension_registrar_unittest.cc
@@ -16,10 +16,12 @@
 #include "extensions/browser/blocklist_extension_prefs.h"
 #include "extensions/browser/extension_prefs.h"
 #include "extensions/browser/extension_registry.h"
+#include "extensions/browser/extension_system.h"
 #include "extensions/browser/extensions_test.h"
 #include "extensions/browser/management_policy.h"
 #include "extensions/browser/test_extension_registry_observer.h"
 #include "extensions/browser/test_extensions_browser_client.h"
+#include "extensions/browser/test_management_policy.h"
 #include "extensions/common/constants.h"
 #include "extensions/common/extension.h"
 #include "extensions/common/extension_builder.h"
@@ -526,6 +528,29 @@ TEST_F(ExtensionRegistrarTest, ReloadExtensionWithNewDisableReason) {
       testing::UnorderedElementsAre(disable_reason::DISABLE_USER_ACTION));
 }
 
+TEST_F(ExtensionRegistrarTest,
+       MustRemainEnabledOverridesPersistedUserDisableOnInstall) {
+  auto* prefs = ExtensionPrefs::Get(browser_context());
+  prefs->AddDisableReason(extension()->id(),
+                          disable_reason::DISABLE_USER_ACTION);
+  EXPECT_THAT(registrar()->GetDisableReasonsOnInstalled(extension().get()),
+              testing::UnorderedElementsAre(disable_reason::DISABLE_USER_ACTION));
+
+  TestManagementPolicyProvider provider(
+      TestManagementPolicyProvider::MUST_REMAIN_ENABLED);
+  auto* policy = ExtensionSystem::Get(browser_context())->management_policy();
+  policy->RegisterProvider(&provider);
+  // Installation must receive no inherited user-disable reasons once the
+  // application requires this extension to remain enabled.
+  EXPECT_TRUE(
+      registrar()->GetDisableReasonsOnInstalled(extension().get()).empty());
+  policy->UnregisterProvider(&provider);
+
+  // Ordinary extensions continue to inherit the user's stored disable reason.
+  EXPECT_THAT(registrar()->GetDisableReasonsOnInstalled(extension().get()),
+              testing::UnorderedElementsAre(disable_reason::DISABLE_USER_ACTION));
+}
+
 TEST_F(ExtensionRegistrarTest, RemoveReloadedExtension) {
   AddEnabledExtension();
   ReloadEnabledExtension();
