diff --git a/chrome/browser/ui/page_action/action_ids.h b/chrome/browser/ui/page_action/action_ids.h
index 31b76346aa..7ce262f38f 100644
--- a/chrome/browser/ui/page_action/action_ids.h
+++ b/chrome/browser/ui/page_action/action_ids.h
@@ -56,6 +56,7 @@ inline constexpr auto kActionIds = std::to_array<actions::ActionId>({
     kActionFakePageActionForDebug,
     kActionWalletReminderNotice,
     kActionGmailOtpOptIn,
+    kActionWindriseSlot,
 
     // Add any new page actions before this line to ensure that the bookmark
     // star is always the right-most page action.
