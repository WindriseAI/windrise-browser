diff --git a/components/infobars/core/infobar_delegate.h b/components/infobars/core/infobar_delegate.h
index 5c0c180..9a54db9 100644
--- a/components/infobars/core/infobar_delegate.h
+++ b/components/infobars/core/infobar_delegate.h
@@ -16,7 +16,6 @@
 class ConfirmInfoBarDelegate;
 class ThemeInstalledInfoBarDelegate;
 
-
 namespace translate {
 class TranslateInfoBarDelegate;
 }
@@ -213,6 +212,9 @@ class InfoBarDelegate {
     FORMS_AI_PRIVATE_INFERENCE_INFOBAR_DELEGATE_IOS = 138,
     PASSWORD_SAVED_INFOBAR_DELEGATE_IOS = 139,
     CONTEXTUAL_CUE_INFOBAR_DELEGATE_IOS = 140,
+    // BrowserOS: agent installation infobars.
+    BROWSEROS_AGENT_INSTALLING_INFOBAR_DELEGATE = 141,
+    BROWSEROS_EXTENSION_INFOBAR_DELEGATE = 142,
   };
   // LINT.ThenChange(//tools/metrics/histograms/metadata/browser/enums.xml:InfoBarIdentifier)
 
