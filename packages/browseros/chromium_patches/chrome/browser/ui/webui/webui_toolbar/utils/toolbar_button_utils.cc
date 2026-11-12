diff --git a/chrome/browser/ui/webui/webui_toolbar/utils/toolbar_button_utils.cc b/chrome/browser/ui/webui/webui_toolbar/utils/toolbar_button_utils.cc
index 57f46e7aa5..b3836165d2 100644
--- a/chrome/browser/ui/webui/webui_toolbar/utils/toolbar_button_utils.cc
+++ b/chrome/browser/ui/webui/webui_toolbar/utils/toolbar_button_utils.cc
@@ -543,6 +543,8 @@ actions::ActionId MojomPageActionIdToActionId(
       return kActionShowPaymentsBubbleOrPage;
     case MojomPageActionId::kActionSidePanelShowContextualTasks:
       return kActionSidePanelShowContextualTasks;
+    case MojomPageActionId::kActionWindriseSlot:
+      return kActionWindriseSlot;
     case MojomPageActionId::kActionBookmarkThisTab:
       return kActionBookmarkThisTab;
     case MojomPageActionId::kActionFederation:
@@ -621,6 +623,8 @@ MojomPageActionId ActionIdToMojomPageActionId(actions::ActionId action_id) {
       return MojomPageActionId::kActionShowPaymentsBubbleOrPage;
     case kActionSidePanelShowContextualTasks:
       return MojomPageActionId::kActionSidePanelShowContextualTasks;
+    case kActionWindriseSlot:
+      return MojomPageActionId::kActionWindriseSlot;
     case kActionBookmarkThisTab:
       return MojomPageActionId::kActionBookmarkThisTab;
     case kActionFederation:
