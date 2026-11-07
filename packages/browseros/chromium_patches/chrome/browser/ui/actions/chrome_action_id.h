diff --git a/chrome/browser/ui/actions/chrome_action_id.h b/chrome/browser/ui/actions/chrome_action_id.h
index 4bcf8e9afd..4d5b36672c 100644
--- a/chrome/browser/ui/actions/chrome_action_id.h
+++ b/chrome/browser/ui/actions/chrome_action_id.h
@@ -88,6 +88,7 @@
   E(kActionShowMemorySaverChip) \
   E(kActionShowJsOptimizationsIcon) \
   E(kActionShowCookieControls) \
+  E(kActionWindriseSlot) \
   E(kActionShowZoomBubble) \
   E(kActionAddNewTabToGroup, IDC_ADD_NEW_TAB_TO_GROUP) \
   E(kActionCreateNewTabGroup, IDC_CREATE_NEW_TAB_GROUP) \
@@ -353,7 +354,9 @@
   E(kActionSidePanelShowSideSearch) \
   E(kActionSidePanelShowMerchantTrust) \
   E(kActionSidePanelShowTabsFromOtherDevices, \
-    IDC_SHOW_TABS_FROM_OTHER_DEVICES_SIDE_PANEL)
+    IDC_SHOW_TABS_FROM_OTHER_DEVICES_SIDE_PANEL) \
+  E(kActionSidePanelShowThirdPartyLlm) \
+  E(kActionBrowserOSAgent)
 
 #define TOOLBAR_PINNABLE_ACTION_IDS \
   E(kActionHome, IDC_HOME) \
