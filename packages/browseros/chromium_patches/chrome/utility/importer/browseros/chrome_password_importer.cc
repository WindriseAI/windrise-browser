diff --git a/chrome/utility/importer/browseros/chrome_password_importer.cc b/chrome/utility/importer/browseros/chrome_password_importer.cc
new file mode 100644
index 0000000000000..3aaeb073d7554
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_password_importer.cc
@@ -0,0 +1,169 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome password importer implementation
+
+#include "chrome/utility/importer/browseros/chrome_password_importer.h"
+
+#include "base/files/file_util.h"
+#include "base/logging.h"
+#include "base/strings/utf_string_conversions.h"
+#include "chrome/utility/importer/browseros/chrome_decryptor.h"
+#include "chrome/utility/importer/browseros/chrome_importer_utils.h"
+#include "sql/database.h"
+#include "sql/statement.h"
+#include "url/gurl.h"
+
+namespace browseros_importer {
+
+namespace {
+
+// Database tag - reuse ChromeImporter tag which is registered in histograms.xml
+inline constexpr sql::Database::Tag kDatabaseTag{"ChromeImporter"};
+
+constexpr char kLoginDataFilename[] = "Login Data";
+
+}  // namespace
+
+std::vector<user_data_importer::ImportedPasswordForm> ImportChromePasswords(
+    const base::FilePath& profile_path,
+    const std::string& encryption_key,
+    ImportItemStatus* status) {
+  std::vector<user_data_importer::ImportedPasswordForm> passwords;
+
+  // Path to Login Data database
+  base::FilePath login_data_path = profile_path.AppendASCII(kLoginDataFilename);
+  // Copy to temp location first so an exclusive lock is classified as kLocked.
+  ImportItemStatus copy_status = ImportItemStatus::kError;
+  base::FilePath temp_db_path =
+      CopyDatabaseToTemp(login_data_path, &copy_status);
+  if (temp_db_path.empty()) {
+    if (status) {
+      *status = copy_status;
+    }
+    return passwords;
+  }
+
+  if (encryption_key.empty()) {
+    LOG(WARNING) << "windrise: Chrome encryption key unavailable; "
+                 << "skipping password import";
+    DeleteDatabaseTemp(temp_db_path);
+    if (status) {
+      *status = ImportItemStatus::kError;
+    }
+    return passwords;
+  }
+
+  // Open database
+  sql::Database db(kDatabaseTag);
+  if (!db.Open(temp_db_path)) {
+    LOG(WARNING) << "windrise: Failed to open Login Data database";
+    if (status) {
+      *status = ClassifySqliteError(db.GetErrorCode());
+    }
+    DeleteDatabaseTemp(temp_db_path);
+    return passwords;
+  }
+
+  // Count of passwords skipped because they use App-Bound Encryption.
+  int skipped_app_bound = 0;
+
+  // Query logins table - use scope block to ensure statement is destroyed
+  // before db.Close() to avoid DCHECK failure
+  {
+    const char kQuery[] =
+        "SELECT origin_url, action_url, username_element, username_value, "
+        "password_element, password_value, signon_realm, blacklisted_by_user, "
+        "scheme FROM logins";
+
+    sql::Statement statement(db.GetUniqueStatement(kQuery));
+    if (!statement.is_valid()) {
+      LOG(WARNING) << "windrise: Failed to prepare Login Data query";
+      if (status) {
+        *status = ClassifySqliteError(db.GetErrorCode());
+      }
+      db.Close();
+      DeleteDatabaseTemp(temp_db_path);
+      return passwords;
+    }
+
+    while (statement.Step()) {
+      std::string origin_url = statement.ColumnString(0);
+      std::string action_url = statement.ColumnString(1);
+      std::u16string username_element = statement.ColumnString16(2);
+      std::u16string username_value = statement.ColumnString16(3);
+      std::u16string password_element = statement.ColumnString16(4);
+
+      // password_value is a BLOB - encrypted
+      std::string encrypted_password = statement.ColumnBlobAsString(5);
+
+      std::string signon_realm = statement.ColumnString(6);
+      bool blacklisted = statement.ColumnBool(7);
+      int scheme = statement.ColumnInt(8);
+
+      // Decrypt password
+      std::string decrypted_password;
+      if (!encrypted_password.empty()) {
+        DecryptResult decrypt_result = DecryptChromeValue(
+            encrypted_password, encryption_key, &decrypted_password);
+        if (decrypt_result == DecryptResult::kAppBoundUnsupported) {
+          // App-Bound (v20) password; not decryptable outside Chrome. Skip.
+          ++skipped_app_bound;
+          continue;
+        }
+        if (decrypt_result != DecryptResult::kSuccess) {
+          LOG(WARNING) << "windrise: Failed to decrypt password for: "
+                       << origin_url;
+          continue;
+        }
+      }
+
+      // Create ImportedPasswordForm
+      user_data_importer::ImportedPasswordForm form;
+
+      // Set scheme
+      if (scheme == 0) {
+        form.scheme = user_data_importer::ImportedPasswordForm::Scheme::kHtml;
+      } else {
+        form.scheme = user_data_importer::ImportedPasswordForm::Scheme::kBasic;
+      }
+
+      form.signon_realm = signon_realm;
+      form.url = GURL(origin_url);
+      form.action = GURL(action_url);
+      form.username_element = username_element;
+      form.username_value = username_value;
+      form.password_element = password_element;
+      form.password_value = base::UTF8ToUTF16(decrypted_password);
+      form.blocked_by_user = blacklisted;
+
+      passwords.push_back(std::move(form));
+    }
+    if (!statement.Succeeded()) {
+      LOG(WARNING) << "windrise: Login Data SQLite query execution failed";
+      if (status) {
+        *status = ClassifySqliteError(db.GetErrorCode());
+      }
+      passwords.clear();
+      db.Close();
+      DeleteDatabaseTemp(temp_db_path);
+      return passwords;
+    }
+  }  // statement destroyed here
+
+  db.Close();
+  DeleteDatabaseTemp(temp_db_path);
+
+  LOG(INFO) << "windrise: Imported " << passwords.size() << " passwords";
+  if (skipped_app_bound > 0) {
+    LOG(WARNING) << "windrise: Skipped " << skipped_app_bound
+                 << " App-Bound (v20) passwords that require Chrome's SYSTEM "
+                    "elevation service and cannot be imported.";
+  }
+
+  if (status) {
+    *status = passwords.empty() ? ImportItemStatus::kEmpty
+                                : ImportItemStatus::kSuccess;
+  }
+  return passwords;
+}
+
+}  // namespace browseros_importer
