diff --git a/chrome/utility/importer/browseros/chrome_password_importer.h b/chrome/utility/importer/browseros/chrome_password_importer.h
new file mode 100644
index 0000000000000..7111af4e124b5
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_password_importer.h
@@ -0,0 +1,25 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome password importer interface
+
+#ifndef CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_PASSWORD_IMPORTER_H_
+#define CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_PASSWORD_IMPORTER_H_
+
+#include <string>
+#include <vector>
+
+#include "base/files/file_path.h"
+#include "chrome/utility/importer/browseros/chrome_importer_utils.h"
+#include "components/user_data_importer/common/importer_data_types.h"
+
+namespace browseros_importer {
+
+// Import passwords from Chrome/Chromium-Edge Login Data database.
+// Populates |status| when provided to distinguish empty vs locked vs error.
+std::vector<user_data_importer::ImportedPasswordForm> ImportChromePasswords(
+    const base::FilePath& profile_path,
+    const std::string& encryption_key,
+    ImportItemStatus* status = nullptr);
+
+}  // namespace browseros_importer
+
+#endif  // CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_PASSWORD_IMPORTER_H_
