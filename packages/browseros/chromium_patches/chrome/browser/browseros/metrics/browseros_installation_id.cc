diff --git a/chrome/browser/browseros/metrics/browseros_installation_id.cc b/chrome/browser/browseros/metrics/browseros_installation_id.cc
new file mode 100644
index 0000000000000000000000000000000000000000..2d9259faba70aba351344624fcb70d77af97c2cf
--- /dev/null
+++ b/chrome/browser/browseros/metrics/browseros_installation_id.cc
@@ -0,0 +1,185 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/metrics/browseros_installation_id.h"
+
+#include <optional>
+#include <string>
+
+#include "base/base_paths.h"
+#include "base/files/file.h"
+#include "base/files/file_enumerator.h"
+#include "base/files/file_util.h"
+#include "base/functional/bind.h"
+#include "base/functional/callback_helpers.h"
+#include "base/json/json_reader.h"
+#include "base/json/json_writer.h"
+#include "base/logging.h"
+#include "base/path_service.h"
+#include "base/uuid.h"
+#include "base/values.h"
+#include "build/build_config.h"
+
+#if BUILDFLAG(IS_POSIX)
+#include <unistd.h>
+#endif
+
+namespace browseros_metrics {
+namespace {
+
+constexpr base::FilePath::StringViewType kInstallationFileName =
+    FILE_PATH_LITERAL("installation.json");
+constexpr char kInstallIdKey[] = "install_id";
+
+std::optional<std::string> ReadInstallationId(
+    const base::FilePath& installation_path) {
+  std::string contents;
+  if (!base::ReadFileToString(installation_path, &contents)) {
+    return std::nullopt;
+  }
+
+  std::optional<base::DictValue> installation =
+      base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
+  const std::string* install_id =
+      installation ? installation->FindString(kInstallIdKey) : nullptr;
+  if (!install_id ||
+      !base::Uuid::ParseCaseInsensitive(*install_id).is_valid()) {
+    LOG(ERROR) << "windrise: Invalid installation identity file: "
+               << installation_path;
+    return std::nullopt;
+  }
+  return *install_id;
+}
+
+bool PublishInstallationFile(const base::FilePath& temporary_path,
+                             const base::FilePath& installation_path) {
+#if BUILDFLAG(IS_WIN)
+  return base::CreateWinHardLink(installation_path, temporary_path);
+#elif BUILDFLAG(IS_POSIX)
+  return ::link(temporary_path.value().c_str(),
+                installation_path.value().c_str()) == 0;
+#else
+  return false;
+#endif
+}
+
+constexpr base::FilePath::StringViewType kLegacyProductStateDirName =
+    FILE_PATH_LITERAL(".browseros");
+
+// Returns ~/.browseros only when it exists, differs from the current state
+// directory, and contains solely installation.json (protecting any standalone
+// upstream installation that has db/, sessions/, etc.).
+base::FilePath FindSoleLegacyProductStateDir(
+    const base::FilePath& current_state_directory) {
+  base::FilePath home;
+  if (!base::PathService::Get(base::DIR_HOME, &home)) {
+    return base::FilePath();
+  }
+  const base::FilePath legacy_dir = home.Append(kLegacyProductStateDirName);
+  if (legacy_dir == current_state_directory ||
+      !base::DirectoryExists(legacy_dir)) {
+    return base::FilePath();
+  }
+
+  base::FileEnumerator entries(
+      legacy_dir, /*recursive=*/false,
+      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
+  bool saw_installation_file = false;
+  for (base::FilePath entry = entries.Next(); !entry.empty();
+       entry = entries.Next()) {
+    if (entry.BaseName().value() != kInstallationFileName || saw_installation_file) {
+      return base::FilePath();
+    }
+    saw_installation_file = true;
+  }
+  return saw_installation_file ? legacy_dir : base::FilePath();
+}
+
+void RemoveLegacyProductStateDir(const base::FilePath& legacy_dir) {
+  if (legacy_dir.empty()) {
+    return;
+  }
+  base::DeleteFile(legacy_dir.Append(kInstallationFileName));
+  // Non-recursive delete: succeeds only if the directory is now empty.
+  base::DeleteFile(legacy_dir);
+}
+
+}  // namespace
+
+std::optional<std::string> LoadOrCreateInstallationId(
+    const base::FilePath& product_state_directory) {
+  if (product_state_directory.empty()) {
+    return std::nullopt;
+  }
+
+  const base::FilePath legacy_dir =
+      FindSoleLegacyProductStateDir(product_state_directory);
+  const base::FilePath installation_path =
+      product_state_directory.Append(kInstallationFileName);
+  if (base::PathExists(installation_path)) {
+    RemoveLegacyProductStateDir(legacy_dir);
+    return ReadInstallationId(installation_path);
+  }
+
+  if (!base::CreateDirectory(product_state_directory)) {
+    LOG(ERROR) << "windrise: Failed to create product state directory: "
+               << product_state_directory;
+    return std::nullopt;
+  }
+
+  std::optional<std::string> migrated_id;
+  if (!legacy_dir.empty()) {
+    migrated_id =
+        ReadInstallationId(legacy_dir.Append(kInstallationFileName));
+  }
+  const std::string candidate_id =
+      migrated_id.has_value()
+          ? *migrated_id
+          : base::Uuid::GenerateRandomV4().AsLowercaseString();
+  base::DictValue installation;
+  installation.Set(kInstallIdKey, candidate_id);
+  std::optional<std::string> json = base::WriteJson(installation);
+  if (!json.has_value()) {
+    return std::nullopt;
+  }
+  json->push_back('\n');
+
+  base::FilePath temporary_path;
+  if (!base::CreateTemporaryFileInDir(product_state_directory,
+                                      &temporary_path)) {
+    LOG(ERROR) << "windrise: Failed to create temporary installation file";
+    return std::nullopt;
+  }
+  base::ScopedClosureRunner delete_temporary_file(base::BindOnce(
+      [](base::FilePath path) { base::DeleteFile(path); }, temporary_path));
+
+  if (!base::WriteFile(temporary_path, *json)) {
+    LOG(ERROR) << "windrise: Failed to write temporary installation file";
+    return std::nullopt;
+  }
+  base::File temporary_file(temporary_path,
+                            base::File::FLAG_OPEN | base::File::FLAG_WRITE);
+  if (!temporary_file.IsValid() || !temporary_file.Flush()) {
+    LOG(ERROR) << "windrise: Failed to flush temporary installation file";
+    return std::nullopt;
+  }
+  temporary_file.Close();
+
+  // A hard link publishes the complete temporary file only when the
+  // destination is absent. If another process won the race, adopt its ID.
+  if (PublishInstallationFile(temporary_path, installation_path)) {
+    RemoveLegacyProductStateDir(legacy_dir);
+    return candidate_id;
+  }
+  if (base::PathExists(installation_path)) {
+    RemoveLegacyProductStateDir(legacy_dir);
+    return ReadInstallationId(installation_path);
+  }
+
+  LOG(ERROR) << "windrise: Failed to publish installation identity: "
+             << installation_path;
+  return std::nullopt;
+}
+
+}  // namespace browseros_metrics
