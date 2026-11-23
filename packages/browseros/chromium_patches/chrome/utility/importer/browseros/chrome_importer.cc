diff --git a/chrome/utility/importer/browseros/chrome_importer.cc b/chrome/utility/importer/browseros/chrome_importer.cc
new file mode 100644
index 0000000000000..3674707cab113
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_importer.cc
@@ -0,0 +1,302 @@
+// Copyright 2023 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/utility/importer/browseros/chrome_importer.h"
+
+#include "base/logging.h"
+#include "chrome/common/importer/importer_bridge.h"
+#include "chrome/grit/generated_resources.h"
+#include "chrome/utility/importer/browseros/chrome_autofill_importer.h"
+#include "chrome/utility/importer/browseros/chrome_bookmarks_importer.h"
+#include "chrome/utility/importer/browseros/chrome_cookie_importer.h"
+#include "chrome/utility/importer/browseros/chrome_decryptor.h"
+#include "chrome/utility/importer/browseros/chrome_extensions_importer.h"
+#include "chrome/utility/importer/browseros/chrome_history_importer.h"
+#include "chrome/utility/importer/browseros/chrome_password_importer.h"
+#include "components/user_data_importer/common/importer_data_types.h"
+#include "ui/base/l10n/l10n_util.h"
+
+ChromeImporter::ChromeImporter() = default;
+
+ChromeImporter::~ChromeImporter() = default;
+
+void ChromeImporter::StartImport(
+    const user_data_importer::SourceProfile& source_profile,
+    uint16_t items,
+    ImporterBridge* bridge) {
+  bridge_ = bridge;
+  source_path_ = source_profile.source_path;
+  source_browser_name_ = source_profile.importer_name;
+  source_profile_name_ = source_profile.profile;
+  chrome_encryption_key_.clear();
+
+  bridge_->NotifyStarted();
+
+  if ((items & (user_data_importer::PASSWORDS | user_data_importer::COOKIES)) &&
+      !cancelled()) {
+    browseros_importer::KeyExtractionResult key_result;
+    chrome_encryption_key_ =
+        browseros_importer::ExtractChromeKey(source_path_, &key_result);
+    if (chrome_encryption_key_.empty()) {
+      LOG(WARNING) << "windrise: Failed to extract Chrome encryption key, "
+                   << "result: " << static_cast<int>(key_result);
+    }
+  }
+
+  uint16_t succeeded_items = user_data_importer::NONE;
+  uint16_t empty_items = user_data_importer::NONE;
+  uint16_t locked_items = user_data_importer::NONE;
+  uint16_t failed_items = user_data_importer::NONE;
+
+  auto run_item = [&](user_data_importer::ImportItem item, auto&& fn) {
+    if ((items & item) == 0 || cancelled()) {
+      return;
+    }
+    bridge_->NotifyItemStarted(item);
+    const browseros_importer::ImportItemStatus status = fn();
+    if (cancelled()) {
+      return;
+    }
+    switch (status) {
+      case browseros_importer::ImportItemStatus::kSuccess:
+        succeeded_items |= static_cast<uint16_t>(item);
+        bridge_->NotifyItemEnded(item);
+        break;
+      case browseros_importer::ImportItemStatus::kEmpty:
+        empty_items |= static_cast<uint16_t>(item);
+        bridge_->NotifyItemEnded(item);
+        break;
+      case browseros_importer::ImportItemStatus::kLocked:
+        // Never send NotifyItemEnded when a database/file is locked so the
+        // host never misclassifies a locked import as succeeded.
+        locked_items |= static_cast<uint16_t>(item);
+        break;
+      case browseros_importer::ImportItemStatus::kError:
+        failed_items |= static_cast<uint16_t>(item);
+        break;
+    }
+  };
+
+  run_item(user_data_importer::HISTORY, [this]() { return ImportHistory(); });
+  run_item(user_data_importer::FAVORITES,
+           [this]() { return ImportBookmarks(); });
+  run_item(user_data_importer::PASSWORDS,
+           [this]() { return ImportPasswords(); });
+  run_item(user_data_importer::COOKIES, [this]() { return ImportCookies(); });
+  run_item(user_data_importer::AUTOFILL_FORM_DATA,
+           [this]() { return ImportAutofillFormData(); });
+  run_item(user_data_importer::EXTENSIONS,
+           [this]() { return ImportExtensions(); });
+
+  const bool has_completed = (succeeded_items != user_data_importer::NONE);
+  bool overall_succeeded = (locked_items == user_data_importer::NONE &&
+                            failed_items == user_data_importer::NONE);
+  std::string error_msg;
+  if (locked_items != user_data_importer::NONE) {
+    overall_succeeded = false;
+    error_msg = has_completed ? "partial_source_browser_locked"
+                              : "source_browser_locked";
+  } else if (failed_items != user_data_importer::NONE) {
+    overall_succeeded = false;
+    error_msg = has_completed ? "partial_import_failed" : "import_failed";
+  } else if (!has_completed && empty_items != user_data_importer::NONE) {
+    error_msg = "empty_source";
+  }
+
+  bridge_->NotifyEndedWithResult(overall_succeeded, error_msg);
+  chrome_encryption_key_.clear();
+}
+
+browseros_importer::ImportItemStatus ChromeImporter::ImportHistory() {
+  LOG(INFO) << "windrise: Starting history import";
+
+  browseros_importer::ImportItemStatus status =
+      browseros_importer::ImportItemStatus::kEmpty;
+  std::vector<user_data_importer::ImporterURLRow> rows =
+      browseros_importer::ImportChromeHistory(source_path_, &status);
+
+  if (status == browseros_importer::ImportItemStatus::kLocked ||
+      status == browseros_importer::ImportItemStatus::kError) {
+    return status;
+  }
+
+  if (rows.empty()) {
+    LOG(INFO) << "windrise: No history to import";
+    return browseros_importer::ImportItemStatus::kEmpty;
+  }
+
+  LOG(INFO) << "windrise: Importing " << rows.size() << " history items";
+
+  if (!cancelled()) {
+    bridge_->SetHistoryItems(rows,
+                             user_data_importer::VISIT_SOURCE_CHROME_IMPORTED);
+  }
+
+  LOG(INFO) << "windrise: History import complete";
+  return browseros_importer::ImportItemStatus::kSuccess;
+}
+
+browseros_importer::ImportItemStatus ChromeImporter::ImportBookmarks() {
+  LOG(INFO) << "windrise: Starting bookmarks import";
+
+  browseros_importer::ImportItemStatus status =
+      browseros_importer::ImportItemStatus::kEmpty;
+  browseros_importer::ChromeBookmarksResult result =
+      browseros_importer::ImportChromeBookmarks(source_path_, &status);
+
+  if (status == browseros_importer::ImportItemStatus::kLocked ||
+      status == browseros_importer::ImportItemStatus::kError) {
+    return status;
+  }
+
+  if (!result.bookmarks.empty() && !cancelled()) {
+    LOG(INFO) << "windrise: Importing " << result.bookmarks.size()
+              << " bookmarks";
+    std::u16string folder_name =
+        source_browser_name_.empty()
+            ? l10n_util::GetStringUTF16(IDS_IMPORT_FROM_CHROME)
+            : source_browser_name_;
+    if (!source_profile_name_.empty()) {
+      folder_name += u" (";
+      folder_name += source_profile_name_;
+      folder_name += u")";
+    }
+    bridge_->AddBookmarks(result.bookmarks, folder_name);
+  } else {
+    LOG(INFO) << "windrise: No bookmarks to import";
+  }
+
+  if (!result.favicons.empty() && !cancelled()) {
+    LOG(INFO) << "windrise: Importing " << result.favicons.size()
+              << " favicons";
+    bridge_->SetFavicons(result.favicons);
+  }
+
+  LOG(INFO) << "windrise: Bookmarks import complete";
+  return result.bookmarks.empty()
+             ? browseros_importer::ImportItemStatus::kEmpty
+             : browseros_importer::ImportItemStatus::kSuccess;
+}
+
+browseros_importer::ImportItemStatus ChromeImporter::ImportPasswords() {
+  LOG(INFO) << "windrise: Starting password import";
+
+  browseros_importer::ImportItemStatus status =
+      browseros_importer::ImportItemStatus::kEmpty;
+  std::vector<user_data_importer::ImportedPasswordForm> passwords =
+      browseros_importer::ImportChromePasswords(
+          source_path_, chrome_encryption_key_, &status);
+
+  if (status == browseros_importer::ImportItemStatus::kLocked ||
+      status == browseros_importer::ImportItemStatus::kError) {
+    return status;
+  }
+
+  if (passwords.empty()) {
+    LOG(INFO) << "windrise: No passwords to import";
+    return browseros_importer::ImportItemStatus::kEmpty;
+  }
+
+  LOG(INFO) << "windrise: Importing " << passwords.size() << " passwords";
+
+  for (const auto& password : passwords) {
+    if (cancelled()) {
+      break;
+    }
+    bridge_->SetPasswordForm(password);
+  }
+
+  LOG(INFO) << "windrise: Password import complete";
+  return browseros_importer::ImportItemStatus::kSuccess;
+}
+
+browseros_importer::ImportItemStatus ChromeImporter::ImportCookies() {
+  LOG(INFO) << "windrise: Starting cookie import";
+
+  browseros_importer::ImportItemStatus status =
+      browseros_importer::ImportItemStatus::kEmpty;
+  std::vector<browseros_importer::ImportedCookieEntry> cookies =
+      browseros_importer::ImportChromeCookies(
+          source_path_, chrome_encryption_key_, &status);
+
+  if (status == browseros_importer::ImportItemStatus::kLocked ||
+      status == browseros_importer::ImportItemStatus::kError) {
+    return status;
+  }
+
+  if (cookies.empty()) {
+    LOG(INFO) << "windrise: No cookies to import";
+    return browseros_importer::ImportItemStatus::kEmpty;
+  }
+
+  LOG(INFO) << "windrise: Importing " << cookies.size() << " cookies";
+
+  for (const auto& cookie : cookies) {
+    if (cancelled()) {
+      break;
+    }
+    bridge_->SetCookie(cookie);
+  }
+
+  LOG(INFO) << "windrise: Cookie import complete";
+  return browseros_importer::ImportItemStatus::kSuccess;
+}
+
+browseros_importer::ImportItemStatus ChromeImporter::ImportAutofillFormData() {
+  LOG(INFO) << "windrise: Starting autofill import";
+
+  browseros_importer::ImportItemStatus status =
+      browseros_importer::ImportItemStatus::kEmpty;
+  std::vector<ImporterAutofillFormDataEntry> entries =
+      browseros_importer::ImportChromeAutofill(source_path_, &status);
+
+  if (status == browseros_importer::ImportItemStatus::kLocked ||
+      status == browseros_importer::ImportItemStatus::kError) {
+    return status;
+  }
+
+  if (entries.empty()) {
+    LOG(INFO) << "windrise: No autofill entries to import";
+    return browseros_importer::ImportItemStatus::kEmpty;
+  }
+
+  LOG(INFO) << "windrise: Importing " << entries.size()
+            << " autofill entries";
+
+  if (!cancelled()) {
+    bridge_->SetAutofillFormData(entries);
+  }
+
+  LOG(INFO) << "windrise: Autofill import complete";
+  return browseros_importer::ImportItemStatus::kSuccess;
+}
+
+browseros_importer::ImportItemStatus ChromeImporter::ImportExtensions() {
+  LOG(INFO) << "windrise: Starting extensions import";
+
+  browseros_importer::ImportItemStatus status =
+      browseros_importer::ImportItemStatus::kEmpty;
+  std::vector<std::string> extension_ids =
+      browseros_importer::ImportChromeExtensions(source_path_, &status);
+
+  if (status == browseros_importer::ImportItemStatus::kLocked ||
+      status == browseros_importer::ImportItemStatus::kError) {
+    return status;
+  }
+
+  if (extension_ids.empty()) {
+    LOG(INFO) << "windrise: No extensions to import";
+    return browseros_importer::ImportItemStatus::kEmpty;
+  }
+
+  LOG(INFO) << "windrise: Importing " << extension_ids.size()
+            << " extensions";
+
+  if (!cancelled()) {
+    bridge_->SetExtensions(extension_ids);
+  }
+
+  LOG(INFO) << "windrise: Extensions import complete";
+  return browseros_importer::ImportItemStatus::kSuccess;
+}
