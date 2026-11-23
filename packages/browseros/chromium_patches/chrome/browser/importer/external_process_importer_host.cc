diff --git a/chrome/browser/importer/external_process_importer_host.cc b/chrome/browser/importer/external_process_importer_host.cc
--- a/chrome/browser/importer/external_process_importer_host.cc
+++ b/chrome/browser/importer/external_process_importer_host.cc
@@ -81,6 +81,14 @@ void ExternalProcessImporterHost::NotifyImportItemEnded(
     observer_->ImportItemEnded(item);
 }

+void ExternalProcessImporterHost::NotifyImportEndedWithResult(
+    bool succeeded,
+    const std::string& error_msg) {
+  if (observer_ && (!succeeded || !error_msg.empty()))
+    observer_->ImportError(error_msg.empty() ? "import_failed" : error_msg);
+  NotifyImportEnded();
+}
+
 void ExternalProcessImporterHost::NotifyImportEnded() {
   CHECK_CURRENTLY_ON(BrowserThread::UI, base::NotFatalUntil::M161);
   firefox_lock_.reset();
