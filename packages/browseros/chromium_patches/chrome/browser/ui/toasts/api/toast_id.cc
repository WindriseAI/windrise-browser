diff --git a/chrome/browser/ui/toasts/api/toast_id.cc b/chrome/browser/ui/toasts/api/toast_id.cc
index ba68484..c4d9694 100644
--- a/chrome/browser/ui/toasts/api/toast_id.cc
+++ b/chrome/browser/ui/toasts/api/toast_id.cc
@@ -135,6 +135,8 @@ std::string_view GetToastName(ToastId toast_id) {
       return "TabGroupFocusExited";
     case ToastId::kEnterpriseCopyFailClosed:
       return "EnterpriseCopyFailClosed";
+    case ToastId::kBrowserOSToast:
+      return "BrowserOSToast";
   }
 
   NOTREACHED();
