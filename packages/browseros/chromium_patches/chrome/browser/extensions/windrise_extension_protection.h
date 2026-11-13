diff --git a/chrome/browser/extensions/windrise_extension_protection.h b/chrome/browser/extensions/windrise_extension_protection.h
new file mode 100644
index 0000000000000000000000000000000000000000..e0b94bf9ef012821d60b9e13937c215f26a0144e
--- /dev/null
+++ b/chrome/browser/extensions/windrise_extension_protection.h
@@ -0,0 +1,22 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+#ifndef CHROME_BROWSER_EXTENSIONS_WINDRISE_EXTENSION_PROTECTION_H_
+#define CHROME_BROWSER_EXTENSIONS_WINDRISE_EXTENSION_PROTECTION_H_
+
+#include "base/callback_list.h"
+#include "base/functional/callback.h"
+
+namespace extensions {
+class Extension;
+class ManagementPolicy;
+// Verifies the fixed identity and every signed resource. No name/ping fallback.
+bool IsVerifiedWindriseExtension(const Extension* extension);
+// Private APIs and native layers also require a live backend-signed lease.
+bool IsAuthorizedWindriseExtension(const Extension* extension);
+void RefreshWindriseAssistantAuthority(const Extension* extension);
+base::CallbackListSubscription AddWindriseAssistantAuthorityObserver(
+    base::RepeatingClosure callback);
+void RegisterWindriseExtensionProtection(ManagementPolicy* policy);
+}  // namespace extensions
+#endif  // CHROME_BROWSER_EXTENSIONS_WINDRISE_EXTENSION_PROTECTION_H_
