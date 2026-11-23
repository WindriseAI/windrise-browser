diff --git a/chrome/browser/importer/importer_progress_observer.h b/chrome/browser/importer/importer_progress_observer.h
--- a/chrome/browser/importer/importer_progress_observer.h
+++ b/chrome/browser/importer/importer_progress_observer.h
@@ -5,6 +5,8 @@
 #ifndef CHROME_BROWSER_IMPORTER_IMPORTER_PROGRESS_OBSERVER_H_
 #define CHROME_BROWSER_IMPORTER_IMPORTER_PROGRESS_OBSERVER_H_

+#include <string>
+
 #include "components/user_data_importer/common/importer_data_types.h"

 namespace importer {
@@ -23,7 +25,12 @@ class ImporterProgressObserver {
   // source profile and is now ready for further processing.
   virtual void ImportItemEnded(user_data_importer::ImportItem item) = 0;

+  // Invoked before ImportEnded() when the importer reports a structured
+  // failure or empty-source outcome (e.g. "source_browser_locked",
+  // "partial_source_browser_locked", "empty_source").
+  virtual void ImportError(const std::string& error_msg) {}
+
   // Invoked when the source profile has been imported.
   virtual void ImportEnded() = 0;

  protected:
