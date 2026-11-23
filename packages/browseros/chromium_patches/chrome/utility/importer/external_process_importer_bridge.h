diff --git a/chrome/utility/importer/external_process_importer_bridge.h b/chrome/utility/importer/external_process_importer_bridge.h
index 2f36e248431a3..6be4b846a312f 100644
--- a/chrome/utility/importer/external_process_importer_bridge.h
+++ b/chrome/utility/importer/external_process_importer_bridge.h
@@ -62,12 +62,19 @@ class ExternalProcessImporterBridge : public ImporterBridge {
   void SetPasswordForm(
       const user_data_importer::ImportedPasswordForm& form) override;

+  void SetCookie(
+      const browseros_importer::ImportedCookieEntry& cookie) override;
+
   void SetAutofillFormData(
       const std::vector<ImporterAutofillFormDataEntry>& entries) override;

+  void SetExtensions(const std::vector<std::string>& extension_ids) override;
+
   void NotifyStarted() override;
   void NotifyItemStarted(user_data_importer::ImportItem item) override;
   void NotifyItemEnded(user_data_importer::ImportItem item) override;
   void NotifyEnded() override;
+  void NotifyEndedWithResult(bool succeeded,
+                             const std::string& error_msg) override;

   std::u16string GetLocalizedString(int message_id) override;
