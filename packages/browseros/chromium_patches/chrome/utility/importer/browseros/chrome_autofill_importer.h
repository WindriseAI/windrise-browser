diff --git a/chrome/utility/importer/browseros/chrome_autofill_importer.h b/chrome/utility/importer/browseros/chrome_autofill_importer.h
new file mode 100644
index 0000000000000..915030187f327
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_autofill_importer.h
@@ -0,0 +1,23 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome autofill importer
+
+#ifndef CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_AUTOFILL_IMPORTER_H_
+#define CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_AUTOFILL_IMPORTER_H_
+
+#include <vector>
+
+#include "base/files/file_path.h"
+#include "chrome/common/importer/importer_autofill_form_data_entry.h"
+#include "chrome/utility/importer/browseros/chrome_importer_utils.h"
+
+namespace browseros_importer {
+
+// Imports autofill form data from Chrome/Chromium-Edge Web Data database.
+// Populates |status| when provided to distinguish empty vs locked vs error.
+std::vector<ImporterAutofillFormDataEntry> ImportChromeAutofill(
+    const base::FilePath& profile_path,
+    ImportItemStatus* status = nullptr);
+
+}  // namespace browseros_importer
+
+#endif  // CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_AUTOFILL_IMPORTER_H_
