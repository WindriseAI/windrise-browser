diff --git a/chrome/browser/extensions/windrise_extension_protection.h b/chrome/browser/extensions/windrise_extension_protection.h
new file mode 100644
index 000000000..bffca3ffa
--- /dev/null
+++ b/chrome/browser/extensions/windrise_extension_protection.h
@@ -0,0 +1,14 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+#ifndef CHROME_BROWSER_EXTENSIONS_WINDRISE_EXTENSION_PROTECTION_H_
+#define CHROME_BROWSER_EXTENSIONS_WINDRISE_EXTENSION_PROTECTION_H_
+
+namespace extensions {
+class Extension;
+class ManagementPolicy;
+// Verifies the fixed identity and every signed resource. No name/ping fallback.
+bool IsVerifiedWindriseExtension(const Extension* extension);
+void RegisterWindriseExtensionProtection(ManagementPolicy* policy);
+}  // namespace extensions
+#endif  // CHROME_BROWSER_EXTENSIONS_WINDRISE_EXTENSION_PROTECTION_H_
