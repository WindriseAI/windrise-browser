diff --git a/chrome/browser/browseros/extensions/browseros_extension_maintainer.h b/chrome/browser/browseros/extensions/browseros_extension_maintainer.h
new file mode 100644
index 0000000000000000000000000000000000000000..82bfd946ec92469f7136fcaede6df9d2e5c6571d
--- /dev/null
+++ b/chrome/browser/browseros/extensions/browseros_extension_maintainer.h
@@ -0,0 +1,41 @@
+// Copyright 2024 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_BROWSEROS_EXTENSIONS_BROWSEROS_EXTENSION_MAINTAINER_H_
+#define CHROME_BROWSER_BROWSEROS_EXTENSIONS_BROWSEROS_EXTENSION_MAINTAINER_H_
+
+#include <string>
+
+#include "base/memory/raw_ptr.h"
+
+class Profile;
+
+namespace browseros {
+
+// Local product housekeeping, scheduled by the loader after discovery/READY.
+// Application packages are the only source of bundled extension updates.
+class BrowserOSExtensionMaintainer {
+ public:
+  explicit BrowserOSExtensionMaintainer(Profile* profile);
+  ~BrowserOSExtensionMaintainer();
+
+  BrowserOSExtensionMaintainer(const BrowserOSExtensionMaintainer&) = delete;
+  BrowserOSExtensionMaintainer& operator=(const BrowserOSExtensionMaintainer&) =
+      delete;
+
+  // Preserves periodic product housekeeping without a second install owner.
+  // The coordinator calls this after READY and on each maintenance cycle.
+  void MaintainInstalledExtensions();
+
+ private:
+  void UninstallInactiveProductExtensions();
+  void ReenableDisabledExtensions();
+  void LogExtensionHealth(const std::string& context);
+
+  raw_ptr<Profile> profile_;
+};
+
+}  // namespace browseros
+
+#endif  // CHROME_BROWSER_BROWSEROS_EXTENSIONS_BROWSEROS_EXTENSION_MAINTAINER_H_
