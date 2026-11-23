diff --git a/chrome/browser/importer/external_process_importer_host.h b/chrome/browser/importer/external_process_importer_host.h
--- a/chrome/browser/importer/external_process_importer_host.h
+++ b/chrome/browser/importer/external_process_importer_host.h
@@ -10,6 +10,7 @@
 #include <stdint.h>

 #include <memory>
+#include <string>

 #include "base/memory/raw_ptr.h"
 #include "base/memory/scoped_refptr.h"
@@ -79,6 +80,8 @@ class ExternalProcessImporterHost
   void NotifyImportStarted();
   void NotifyImportItemStarted(user_data_importer::ImportItem item);
   void NotifyImportItemEnded(user_data_importer::ImportItem item);
+  void NotifyImportEndedWithResult(bool succeeded,
+                                   const std::string& error_msg);
   void NotifyImportEnded();

  private:
