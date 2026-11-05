diff --git a/chrome/browser/ui/toasts/api/toast_id.h b/chrome/browser/ui/toasts/api/toast_id.h
index a347e4b..e3a579d 100644
--- a/chrome/browser/ui/toasts/api/toast_id.h
+++ b/chrome/browser/ui/toasts/api/toast_id.h
@@ -85,7 +85,8 @@ enum class ToastId {
   kTtcNoMicrophoneError = 62,
   kTabGroupFocusExited = 63,
   kEnterpriseCopyFailClosed = 64,
-  kMaxValue = kEnterpriseCopyFailClosed,
+  kBrowserOSToast = 65,
+  kMaxValue = kBrowserOSToast,
 };
 // LINT.ThenChange(/tools/metrics/histograms/metadata/toasts/enums.xml:ToastId)
 
