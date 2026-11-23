diff --git a/chrome/utility/importer/browseros/chrome_extensions_importer.cc b/chrome/utility/importer/browseros/chrome_extensions_importer.cc
new file mode 100644
index 0000000000000..bd13aa92a2373
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_extensions_importer.cc
@@ -0,0 +1,141 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome extensions importer implementation
+
+#include "chrome/utility/importer/browseros/chrome_extensions_importer.h"
+
+#include "base/files/file_util.h"
+#include "base/json/json_reader.h"
+#include "base/logging.h"
+#include "chrome/utility/importer/browseros/chrome_importer_utils.h"
+
+namespace browseros_importer {
+
+namespace {
+
+constexpr char kPreferencesFilename[] = "Preferences";
+constexpr char kSecurePreferencesFilename[] = "Secure Preferences";
+
+std::vector<std::string> GetExtensionsFromPreferencesFile(
+    const base::FilePath& preferences_path,
+    ImportItemStatus* status) {
+  std::vector<std::string> extension_ids;
+
+  std::string preferences_content;
+  ImportItemStatus read_status = ImportItemStatus::kError;
+  if (!ReadFileToStringClassified(preferences_path, &preferences_content,
+                                  &read_status)) {
+    LOG(WARNING) << "windrise: Failed to read "
+                 << preferences_path.BaseName().value();
+    if (status) {
+      *status = read_status;
+    }
+    return extension_ids;
+  }
+
+  std::optional<base::DictValue> preferences = base::JSONReader::ReadDict(
+      preferences_content, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
+  if (!preferences) {
+    LOG(WARNING) << "windrise: Failed to parse JSON from "
+                 << preferences_path.BaseName().value();
+    if (status) {
+      *status = ImportItemStatus::kError;
+    }
+    return extension_ids;
+  }
+
+  const base::DictValue* extensions_dict =
+      preferences->FindDictByDottedPath("extensions.settings");
+  if (!extensions_dict) {
+    if (status) {
+      *status = ImportItemStatus::kEmpty;
+    }
+    return extension_ids;
+  }
+
+  for (const auto [key, value] : *extensions_dict) {
+    if (!value.is_dict()) {
+      continue;
+    }
+
+    const base::DictValue& dict = value.GetDict();
+
+    // Skip default-installed extensions
+    if (dict.FindBool("was_installed_by_default").value_or(false)) {
+      continue;
+    }
+
+    // Only import extensions from Chrome Web Store
+    if (!dict.FindBool("from_webstore").value_or(false)) {
+      continue;
+    }
+
+    extension_ids.push_back(key);
+  }
+
+  if (status) {
+    *status = extension_ids.empty() ? ImportItemStatus::kEmpty
+                                    : ImportItemStatus::kSuccess;
+  }
+  return extension_ids;
+}
+
+}  // namespace
+
+std::vector<std::string> ImportChromeExtensions(
+    const base::FilePath& profile_path,
+    ImportItemStatus* status) {
+  std::vector<std::string> extension_ids;
+
+  base::FilePath preferences_path =
+      profile_path.AppendASCII(kPreferencesFilename);
+  base::FilePath secure_preferences_path =
+      profile_path.AppendASCII(kSecurePreferencesFilename);
+
+  bool any_locked = false;
+  bool any_error = false;
+  bool any_readable = false;
+
+  // Secure Preferences takes priority
+  {
+    ImportItemStatus file_status = ImportItemStatus::kEmpty;
+    extension_ids =
+        GetExtensionsFromPreferencesFile(secure_preferences_path, &file_status);
+    if (file_status == ImportItemStatus::kLocked) {
+      any_locked = true;
+    } else if (file_status == ImportItemStatus::kError) {
+      any_error = true;
+    } else if (file_status == ImportItemStatus::kSuccess) {
+      any_readable = true;
+    }
+  }
+
+  // Also check regular Preferences for additional extensions
+  {
+    ImportItemStatus file_status = ImportItemStatus::kEmpty;
+    std::vector<std::string> pref_extension_ids =
+        GetExtensionsFromPreferencesFile(preferences_path, &file_status);
+    if (file_status == ImportItemStatus::kLocked) {
+      any_locked = true;
+    } else if (file_status == ImportItemStatus::kError) {
+      any_error = true;
+    } else if (file_status == ImportItemStatus::kSuccess) {
+      any_readable = true;
+      extension_ids.insert(extension_ids.end(), pref_extension_ids.begin(),
+                           pref_extension_ids.end());
+    }
+  }
+
+  if (status) {
+    if (any_locked) {
+      *status = ImportItemStatus::kLocked;
+    } else if (!any_readable && any_error) {
+      *status = ImportItemStatus::kError;
+    } else {
+      *status = extension_ids.empty() ? ImportItemStatus::kEmpty
+                                      : ImportItemStatus::kSuccess;
+    }
+  }
+  return extension_ids;
+}
+
+}  // namespace browseros_importer
