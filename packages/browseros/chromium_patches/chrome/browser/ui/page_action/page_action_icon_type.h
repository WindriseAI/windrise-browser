diff --git a/chrome/browser/ui/page_action/page_action_icon_type.h b/chrome/browser/ui/page_action/page_action_icon_type.h
index 3f3af81954..5c58d8f38c 100644
--- a/chrome/browser/ui/page_action/page_action_icon_type.h
+++ b/chrome/browser/ui/page_action/page_action_icon_type.h
@@ -63,7 +63,8 @@ enum class PageActionIconType {
   kFakePageActionForDebug = 48,
   kWalletReminderNotice = 49,
   kGmailOtpOptIn = 50,
-  kMaxValue = kGmailOtpOptIn,
+  kWindriseSlot = 51,
+  kMaxValue = kWindriseSlot,
 };
 // LINT.ThenChange(//tools/metrics/histograms/metadata/page/enums.xml:PageActionIconType)
 
