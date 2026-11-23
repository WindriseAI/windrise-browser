diff --git a/chrome/utility/importer/browseros/chrome_history_importer.h b/chrome/utility/importer/browseros/chrome_history_importer.h
new file mode 100644
index 0000000000000..3de1c03c501a9
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_history_importer.h
@@ -0,0 +1,24 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome history importer
+
+#ifndef CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_HISTORY_IMPORTER_H_
+#define CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_HISTORY_IMPORTER_H_
+
+#include <vector>
+
+#include "base/files/file_path.h"
+#include "chrome/utility/importer/browseros/chrome_importer_utils.h"
+#include "components/user_data_importer/common/importer_url_row.h"
+
+namespace browseros_importer {
+
+// Imports browsing history from Chrome's History database.
+// |profile_path| should be the Chrome/Chromium-Edge profile directory.
+// Populates |status| when provided to distinguish empty vs locked vs error.
+std::vector<user_data_importer::ImporterURLRow> ImportChromeHistory(
+    const base::FilePath& profile_path,
+    ImportItemStatus* status = nullptr);
+
+}  // namespace browseros_importer
+
+#endif  // CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_HISTORY_IMPORTER_H_
