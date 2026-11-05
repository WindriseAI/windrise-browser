diff --git a/chrome/browser/ui/webui/webui_toolbar/utils/toolbar_button_utils.cc b/chrome/browser/ui/webui/webui_toolbar/utils/toolbar_button_utils.cc
index b0d2b38b1c..d9b39daac2 100644
--- a/chrome/browser/ui/webui/webui_toolbar/utils/toolbar_button_utils.cc
+++ b/chrome/browser/ui/webui/webui_toolbar/utils/toolbar_button_utils.cc
@@ -542,6 +542,8 @@ actions::ActionId MojomPageActionIdToActionId(
       return kActionShowPaymentsBubbleOrPage;
     case MojomPageActionId::kActionSidePanelShowContextualTasks:
       return kActionSidePanelShowContextualTasks;
+    case MojomPageActionId::kActionWindriseSlot:
+      return kActionWindriseSlot;
     case MojomPageActionId::kActionBookmarkThisTab:
       return kActionBookmarkThisTab;
     case MojomPageActionId::kActionFederation:
@@ -620,6 +622,8 @@ MojomPageActionId ActionIdToMojomPageActionId(actions::ActionId action_id) {
       return MojomPageActionId::kActionShowPaymentsBubbleOrPage;
     case kActionSidePanelShowContextualTasks:
       return MojomPageActionId::kActionSidePanelShowContextualTasks;
+    case kActionWindriseSlot:
+      return MojomPageActionId::kActionWindriseSlot;
     case kActionBookmarkThisTab:
       return MojomPageActionId::kActionBookmarkThisTab;
     case kActionFederation:
