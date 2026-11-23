diff --git a/chrome/utility/importer/browseros/chrome_importer_utils.cc b/chrome/utility/importer/browseros/chrome_importer_utils.cc
new file mode 100644
index 0000000000000..3000a42871631
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_importer_utils.cc
@@ -0,0 +1,230 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome importer shared utilities
+
+#include "chrome/utility/importer/browseros/chrome_importer_utils.h"
+
+#include "base/files/file.h"
+#include "base/files/file_util.h"
+#include "base/logging.h"
+#include "build/build_config.h"
+
+#if BUILDFLAG(IS_WIN)
+#include <windows.h>
+#elif BUILDFLAG(IS_POSIX)
+#include <errno.h>
+#endif
+
+namespace browseros_importer {
+
+namespace {
+
+void SetStatus(ImportItemStatus* status, ImportItemStatus value) {
+  if (status) {
+    *status = value;
+  }
+}
+
+#if BUILDFLAG(IS_WIN)
+ImportItemStatus ClassifyWinFileError(DWORD win_err,
+                                      const base::FilePath& source_path) {
+  if (win_err == ERROR_SHARING_VIOLATION || win_err == ERROR_LOCK_VIOLATION) {
+    return ImportItemStatus::kLocked;
+  }
+  if (win_err == ERROR_FILE_NOT_FOUND || win_err == ERROR_PATH_NOT_FOUND) {
+    return ImportItemStatus::kEmpty;
+  }
+  // Re-probe read open in case CopyFile/ReadFile overwrote GetLastError().
+  base::File probe(source_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
+  if (!probe.IsValid()) {
+    if (probe.error_details() == base::File::FILE_ERROR_IN_USE) {
+      return ImportItemStatus::kLocked;
+    }
+    if (probe.error_details() == base::File::FILE_ERROR_NOT_FOUND) {
+      return ImportItemStatus::kEmpty;
+    }
+  }
+  return ImportItemStatus::kError;
+}
+#elif BUILDFLAG(IS_POSIX)
+ImportItemStatus ClassifyPosixFileError(int err) {
+  if (err == EBUSY || err == EAGAIN || err == EWOULDBLOCK) {
+    return ImportItemStatus::kLocked;
+  }
+  if (err == ENOENT) {
+    return ImportItemStatus::kEmpty;
+  }
+  return ImportItemStatus::kError;
+}
+#endif
+
+}  // namespace
+
+base::Time ChromeTimeToBaseTime(int64_t chrome_time) {
+  if (chrome_time == 0) {
+    return base::Time();
+  }
+  return base::Time::FromDeltaSinceWindowsEpoch(
+      base::Microseconds(chrome_time));
+}
+
+base::FilePath CopyToTempFile(const base::FilePath& source_path,
+                              ImportItemStatus* status) {
+  // Probe open first so Windows sharing violations or permission errors are
+  // never swallowed by base::PathExists() returning false.
+  {
+    base::File probe(source_path,
+                     base::File::FLAG_OPEN | base::File::FLAG_READ);
+    if (!probe.IsValid()) {
+      const base::File::Error err = probe.error_details();
+      if (err == base::File::FILE_ERROR_NOT_FOUND) {
+        SetStatus(status, ImportItemStatus::kEmpty);
+      } else if (err == base::File::FILE_ERROR_IN_USE) {
+        SetStatus(status, ImportItemStatus::kLocked);
+      } else {
+        SetStatus(status, ImportItemStatus::kError);
+      }
+      return base::FilePath();
+    }
+  }
+
+  base::FilePath temp_path;
+  if (!base::CreateTemporaryFile(&temp_path)) {
+    LOG(WARNING) << "windrise: Failed to create temp file for "
+                 << source_path.BaseName().value();
+    SetStatus(status, ImportItemStatus::kError);
+    return base::FilePath();
+  }
+
+  if (!base::CopyFile(source_path, temp_path)) {
+#if BUILDFLAG(IS_WIN)
+    const DWORD win_err = ::GetLastError();
+    const ImportItemStatus classified =
+        ClassifyWinFileError(win_err, source_path);
+#elif BUILDFLAG(IS_POSIX)
+    const int err = errno;
+    const ImportItemStatus classified = ClassifyPosixFileError(err);
+#else
+    const ImportItemStatus classified = ImportItemStatus::kError;
+#endif
+    LOG(WARNING) << "windrise: Failed to copy "
+                 << source_path.BaseName().value() << " to temp (status="
+                 << static_cast<int>(classified) << ")";
+    base::DeleteFile(temp_path);
+    SetStatus(status, classified);
+    return base::FilePath();
+  }
+
+  SetStatus(status, ImportItemStatus::kSuccess);
+  return temp_path;
+}
+
+base::FilePath CopyDatabaseToTemp(const base::FilePath& db_path,
+                                  ImportItemStatus* status) {
+  ImportItemStatus copy_status = ImportItemStatus::kError;
+  base::FilePath temp_path = CopyToTempFile(db_path, &copy_status);
+  if (temp_path.empty()) {
+    SetStatus(status, copy_status);
+    return base::FilePath();
+  }
+
+  // Also copy the "-wal" sidecar if present. If the WAL is exclusively locked
+  // or unreadable, propagate kLocked/kError rather than silently importing
+  // stale or incomplete data.
+  const base::FilePath::StringType kWalSuffix = FILE_PATH_LITERAL("-wal");
+  base::FilePath wal_src(db_path.value() + kWalSuffix);
+  base::File wal_probe(wal_src, base::File::FLAG_OPEN | base::File::FLAG_READ);
+  const bool wal_exists =
+      wal_probe.IsValid() ||
+      wal_probe.error_details() != base::File::FILE_ERROR_NOT_FOUND;
+  const base::File::Error wal_probe_err =
+      wal_probe.IsValid() ? base::File::FILE_OK : wal_probe.error_details();
+  wal_probe.Close();
+  if (wal_exists) {
+    if (wal_probe_err != base::File::FILE_OK) {
+      const ImportItemStatus wal_status =
+          (wal_probe_err == base::File::FILE_ERROR_IN_USE)
+              ? ImportItemStatus::kLocked
+              : ImportItemStatus::kError;
+      DeleteDatabaseTemp(temp_path);
+      SetStatus(status, wal_status);
+      return base::FilePath();
+    }
+    if (!base::CopyFile(wal_src, base::FilePath(temp_path.value() + kWalSuffix))) {
+#if BUILDFLAG(IS_WIN)
+      const DWORD win_err = ::GetLastError();
+      const ImportItemStatus wal_status =
+          ClassifyWinFileError(win_err, wal_src);
+#elif BUILDFLAG(IS_POSIX)
+      const int err = errno;
+      const ImportItemStatus wal_status = ClassifyPosixFileError(err);
+#else
+      const ImportItemStatus wal_status = ImportItemStatus::kError;
+#endif
+      LOG(WARNING) << "windrise: WAL sidecar copy failed for "
+                   << db_path.BaseName().value() << " (status="
+                   << static_cast<int>(wal_status) << ")";
+      DeleteDatabaseTemp(temp_path);
+      SetStatus(status, wal_status == ImportItemStatus::kEmpty
+                            ? ImportItemStatus::kError
+                            : wal_status);
+      return base::FilePath();
+    }
+  }
+
+  SetStatus(status, ImportItemStatus::kSuccess);
+  return temp_path;
+}
+
+bool ReadFileToStringClassified(const base::FilePath& source_path,
+                                std::string* content,
+                                ImportItemStatus* status) {
+  {
+    base::File probe(source_path,
+                     base::File::FLAG_OPEN | base::File::FLAG_READ);
+    if (!probe.IsValid()) {
+      const base::File::Error err = probe.error_details();
+      if (err == base::File::FILE_ERROR_NOT_FOUND) {
+        SetStatus(status, ImportItemStatus::kEmpty);
+      } else if (err == base::File::FILE_ERROR_IN_USE) {
+        SetStatus(status, ImportItemStatus::kLocked);
+      } else {
+        SetStatus(status, ImportItemStatus::kError);
+      }
+      return false;
+    }
+  }
+  if (!base::ReadFileToString(source_path, content)) {
+#if BUILDFLAG(IS_WIN)
+    const DWORD win_err = ::GetLastError();
+    SetStatus(status, ClassifyWinFileError(win_err, source_path));
+#elif BUILDFLAG(IS_POSIX)
+    const int err = errno;
+    SetStatus(status, ClassifyPosixFileError(err));
+#else
+    SetStatus(status, ImportItemStatus::kError);
+#endif
+    return false;
+  }
+  SetStatus(status, ImportItemStatus::kSuccess);
+  return true;
+}
+
+ImportItemStatus ClassifySqliteError(int sqlite_error_code) {
+  const int primary = sqlite_error_code & 0xFF;
+  // SQLITE_BUSY = 5, SQLITE_LOCKED = 6.
+  if (primary == 5 || primary == 6) {
+    return ImportItemStatus::kLocked;
+  }
+  return ImportItemStatus::kError;
+}
+
+void DeleteDatabaseTemp(const base::FilePath& temp_path) {
+  if (temp_path.empty()) {
+    return;
+  }
+  base::DeleteFile(temp_path);
+  base::DeleteFile(base::FilePath(temp_path.value() + FILE_PATH_LITERAL("-wal")));
+  base::DeleteFile(base::FilePath(temp_path.value() + FILE_PATH_LITERAL("-shm")));
+}
+
+}  // namespace browseros_importer
