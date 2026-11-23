diff --git a/chrome/utility/importer/browseros/chrome_importer_utils.h b/chrome/utility/importer/browseros/chrome_importer_utils.h
new file mode 100644
index 0000000000000..d7334846286cd
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_importer_utils.h
@@ -0,0 +1,60 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome importer shared utilities
+
+#ifndef CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_IMPORTER_UTILS_H_
+#define CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_IMPORTER_UTILS_H_
+
+#include "base/files/file_path.h"
+#include "base/time/time.h"
+
+namespace browseros_importer {
+
+// Classified outcome for reading a single Chrome/Chromium-Edge data source.
+// Distinguishes clean empty profiles from real file-lock / I/O failures so
+// locked databases are never misreported as successful imports.
+enum class ImportItemStatus {
+  kSuccess,
+  kEmpty,
+  kLocked,
+  kError,
+};
+
+// Converts Chrome's internal time format (microseconds since Windows epoch)
+// to base::Time. Returns null time for zero input.
+base::Time ChromeTimeToBaseTime(int64_t chrome_time);
+
+// Probes whether |source_path| exists and is readable without swallowing
+// Windows sharing violations or permission errors into a false "missing" state.
+// Returns kSuccess when readable, kEmpty when absent, kLocked when locked, or
+// kError on other I/O or permission failures.
+ImportItemStatus ProbeFileStatus(const base::FilePath& source_path);
+
+// Copies a file to a temporary location to avoid locking issues when the
+// source browser is running. Returns empty path on failure and populates
+// |status| when provided (kEmpty if missing, kLocked on sharing/lock conflict,
+// kError otherwise). Caller is responsible for deleting the temp file.
+base::FilePath CopyToTempFile(const base::FilePath& source_path,
+                              ImportItemStatus* status = nullptr);
+
+// Copies a SQLite database plus its "-wal" write-ahead-log sidecar (if present)
+// to a temporary location. Returns empty path on failure and populates |status|
+// when provided. Pair with DeleteDatabaseTemp() for cleanup.
+base::FilePath CopyDatabaseToTemp(const base::FilePath& db_path,
+                                  ImportItemStatus* status = nullptr);
+
+// Reads a file into |content| and classifies lock/missing/error outcomes.
+bool ReadFileToStringClassified(const base::FilePath& source_path,
+                                std::string* content,
+                                ImportItemStatus* status = nullptr);
+
+// Maps a SQLite error code from sql::Database::GetErrorCode() into kLocked
+// (SQLITE_BUSY / SQLITE_LOCKED) or kError.
+ImportItemStatus ClassifySqliteError(int sqlite_error_code);
+
+// Deletes a temp database created by CopyDatabaseToTemp(), including its "-wal"
+// and "-shm" sidecars. No-op for an empty path.
+void DeleteDatabaseTemp(const base::FilePath& temp_path);
+
+}  // namespace browseros_importer
+
+#endif  // CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_IMPORTER_UTILS_H_
