diff --git a/chrome/browser/ui/actions/chrome_action_id.h b/chrome/browser/ui/actions/chrome_action_id.h
index b9c8319367e60c5925b83f809a8a8b71d8b56df8..cebc12be82a90be129f6c96f8957e4abc7e6e29e 100644
--- a/chrome/browser/ui/actions/chrome_action_id.h
+++ b/chrome/browser/ui/actions/chrome_action_id.h
@@ -512,7 +512,9 @@
   E(kActionSidePanelShowSideSearch) \
   E(kActionSidePanelShowMerchantTrust) \
   E(kActionSidePanelShowTabsFromOtherDevices, \
-    IDC_SHOW_TABS_FROM_OTHER_DEVICES_SIDE_PANEL)
+    IDC_SHOW_TABS_FROM_OTHER_DEVICES_SIDE_PANEL) \
+  E(kActionSidePanelShowThirdPartyLlm) \
+  E(kActionBrowserOSAgent)
 
 #define TOOLBAR_PINNABLE_ACTION_IDS \
   E(kActionHome, IDC_HOME) \
