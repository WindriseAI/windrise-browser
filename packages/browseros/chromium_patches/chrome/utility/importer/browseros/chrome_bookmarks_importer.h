diff --git a/chrome/utility/importer/browseros/chrome_bookmarks_importer.h b/chrome/utility/importer/browseros/chrome_bookmarks_importer.h
new file mode 100644
index 0000000000000..40dfcc2ce8af8
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_bookmarks_importer.h
@@ -0,0 +1,37 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome bookmarks importer
+
+#ifndef CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_BOOKMARKS_IMPORTER_H_
+#define CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_BOOKMARKS_IMPORTER_H_
+
+#include <vector>
+
+#include "base/files/file_path.h"
+#include "chrome/utility/importer/browseros/chrome_importer_utils.h"
+#include "components/favicon_base/favicon_usage_data.h"
+#include "components/user_data_importer/common/imported_bookmark_entry.h"
+
+namespace browseros_importer {
+
+// Result of bookmark import operation containing both bookmarks and favicons.
+struct ChromeBookmarksResult {
+  ChromeBookmarksResult();
+  ~ChromeBookmarksResult();
+  ChromeBookmarksResult(ChromeBookmarksResult&&);
+  ChromeBookmarksResult& operator=(ChromeBookmarksResult&&);
+
+  std::vector<user_data_importer::ImportedBookmarkEntry> bookmarks;
+  favicon_base::FaviconUsageDataList favicons;
+  ImportItemStatus status = ImportItemStatus::kEmpty;
+};
+
+// Imports bookmarks and favicons from Chrome/Chromium-Edge.
+// Populates |result.status| (and optional |status|) to distinguish empty vs
+// locked vs error.
+ChromeBookmarksResult ImportChromeBookmarks(
+    const base::FilePath& profile_path,
+    ImportItemStatus* status = nullptr);
+
+}  // namespace browseros_importer
+
+#endif  // CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_BOOKMARKS_IMPORTER_H_
