diff --git a/chrome/utility/importer/browseros/chrome_cookie_importer.h b/chrome/utility/importer/browseros/chrome_cookie_importer.h
new file mode 100644
index 0000000000000..b706ed9dfa177
--- /dev/null
+++ b/chrome/utility/importer/browseros/chrome_cookie_importer.h
@@ -0,0 +1,54 @@
+// Copyright 2024 AKW Technology Inc
+// Chrome cookie importer interface
+
+#ifndef CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_COOKIE_IMPORTER_H_
+#define CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_COOKIE_IMPORTER_H_
+
+#include <string>
+#include <vector>
+
+#include "base/files/file_path.h"
+#include "base/time/time.h"
+#include "chrome/utility/importer/browseros/chrome_importer_utils.h"
+#include "net/cookies/cookie_constants.h"
+
+namespace browseros_importer {
+
+// Represents a cookie imported from Chrome's Cookies database.
+// Fields mirror Chrome's cookies table schema (v24+).
+struct ImportedCookieEntry {
+  ImportedCookieEntry();
+  ImportedCookieEntry(const ImportedCookieEntry& other);
+  ImportedCookieEntry(ImportedCookieEntry&& other) noexcept;
+  ImportedCookieEntry& operator=(const ImportedCookieEntry& other);
+  ImportedCookieEntry& operator=(ImportedCookieEntry&& other) noexcept;
+  ~ImportedCookieEntry();
+
+  std::string host_key;
+  std::string name;
+  std::string value;  // Decrypted value
+  std::string path;
+  base::Time expires_utc;
+  base::Time creation_utc;
+  base::Time last_access_utc;
+  base::Time last_update_utc;
+  bool is_secure = false;
+  bool is_httponly = false;
+  net::CookieSameSite same_site = net::CookieSameSite::UNSPECIFIED;
+  net::CookiePriority priority = net::CookiePriority::COOKIE_PRIORITY_MEDIUM;
+  net::CookieSourceScheme source_scheme = net::CookieSourceScheme::kUnset;
+  int source_port = -1;
+  bool is_persistent = true;  // Has expiry date
+};
+
+// Imports cookies from Chrome/Chromium-Edge Cookies database (supporting both
+// modern Network/Cookies and legacy Cookies locations).
+// Populates |status| when provided to distinguish empty vs locked vs error.
+std::vector<ImportedCookieEntry> ImportChromeCookies(
+    const base::FilePath& profile_path,
+    const std::string& encryption_key,
+    ImportItemStatus* status = nullptr);
+
+}  // namespace browseros_importer
+
+#endif  // CHROME_UTILITY_IMPORTER_BROWSEROS_CHROME_COOKIE_IMPORTER_H_
