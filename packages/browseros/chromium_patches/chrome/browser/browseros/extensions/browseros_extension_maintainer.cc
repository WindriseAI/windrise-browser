diff --git a/chrome/browser/browseros/extensions/browseros_extension_maintainer.cc b/chrome/browser/browseros/extensions/browseros_extension_maintainer.cc
new file mode 100644
index 0000000000000000000000000000000000000000..4bd56cb4fbf63f779ac155255c2005e7e833b966
--- /dev/null
+++ b/chrome/browser/browseros/extensions/browseros_extension_maintainer.cc
@@ -0,0 +1,151 @@
+// Copyright 2024 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/extensions/browseros_extension_maintainer.h"
+
+#include <utility>
+
+#include "base/logging.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
+#include "chrome/browser/browseros/metrics/browseros_metrics.h"
+#include "chrome/browser/profiles/profile.h"
+#include "content/public/browser/browser_thread.h"
+#include "extensions/browser/disable_reason.h"
+#include "extensions/browser/extension_prefs.h"
+#include "extensions/browser/extension_registrar.h"
+#include "extensions/browser/extension_registry.h"
+#include "extensions/browser/uninstall_reason.h"
+#include "extensions/common/extension.h"
+
+namespace browseros {
+
+BrowserOSExtensionMaintainer::BrowserOSExtensionMaintainer(Profile* profile)
+    : profile_(profile) {}
+
+BrowserOSExtensionMaintainer::~BrowserOSExtensionMaintainer() = default;
+
+void BrowserOSExtensionMaintainer::MaintainInstalledExtensions() {
+  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
+  UninstallInactiveProductExtensions();
+  ReenableDisabledExtensions();
+  LogExtensionHealth("maintenance");
+}
+
+void BrowserOSExtensionMaintainer::UninstallInactiveProductExtensions() {
+  if (!profile_) {
+    return;
+  }
+
+  extensions::ExtensionRegistry* registry =
+      extensions::ExtensionRegistry::Get(profile_);
+  extensions::ExtensionRegistrar* registrar =
+      extensions::ExtensionRegistrar::Get(profile_);
+
+  if (!registry || !registrar) {
+    return;
+  }
+
+  for (const std::string& id : GetAllBrowserOSExtensionIds()) {
+    if (IsActiveBrowserOSExtension(id)) {
+      continue;
+    }
+
+    const extensions::Extension* ext = registry->GetInstalledExtension(id);
+    if (!ext) {
+      continue;
+    }
+
+    LOG(INFO) << "windrise: Uninstalling inactive product extension " << id;
+
+    std::u16string error;
+    if (!registrar->UninstallExtension(
+            id, extensions::UNINSTALL_REASON_ORPHANED_EXTERNAL_EXTENSION,
+            &error)) {
+      LOG(WARNING) << "windrise: Failed to uninstall " << id << ": " << error;
+    }
+  }
+}
+
+void BrowserOSExtensionMaintainer::ReenableDisabledExtensions() {
+  auto* registry = extensions::ExtensionRegistry::Get(profile_);
+  auto* registrar = extensions::ExtensionRegistrar::Get(profile_);
+  for (const auto& id : GetActiveBrowserOSExtensionIds()) {
+    if (!registry->disabled_extensions().Contains(id)) {
+      continue;
+    }
+    // Keep product extensions usable, but never clear corruption, policy or
+    // other safety disablement just because a maintenance timer fired.
+    const auto reasons =
+        extensions::ExtensionPrefs::Get(profile_)->GetDisableReasons(id);
+    bool can_reenable = true;
+    for (auto reason : reasons) {
+      if (reason != extensions::disable_reason::DISABLE_USER_ACTION &&
+          reason != extensions::disable_reason::DISABLE_EXTERNAL_EXTENSION &&
+          reason != extensions::disable_reason::DISABLE_PERMISSIONS_INCREASE) {
+        can_reenable = false;
+        break;
+      }
+    }
+    if (can_reenable) {
+      LOG(INFO) << "windrise: Re-enabling managed extension " << id;
+      registrar->EnableExtension(id);
+    }
+  }
+}
+
+void BrowserOSExtensionMaintainer::LogExtensionHealth(
+    const std::string& context) {
+  if (!profile_) {
+    return;
+  }
+
+  extensions::ExtensionRegistry* registry =
+      extensions::ExtensionRegistry::Get(profile_);
+  extensions::ExtensionPrefs* prefs = extensions::ExtensionPrefs::Get(profile_);
+
+  if (!registry || !prefs) {
+    return;
+  }
+
+  for (const std::string& id : GetActiveBrowserOSExtensionIds()) {
+    if (registry->enabled_extensions().Contains(id)) {
+      continue;
+    }
+
+    std::string state;
+    base::DictValue properties;
+    properties.Set("extension_id", id);
+    properties.Set("context", context);
+
+    if (registry->disabled_extensions().Contains(id)) {
+      state = "disabled";
+
+      extensions::DisableReasonSet reasons = prefs->GetDisableReasons(id);
+      int bitmask = 0;
+      for (extensions::disable_reason::DisableReason reason : reasons) {
+        bitmask |= static_cast<int>(reason);
+      }
+      properties.Set("disable_reasons_bitmask", bitmask);
+
+    } else if (registry->blocklisted_extensions().Contains(id)) {
+      state = "blocklisted";
+    } else if (registry->blocked_extensions().Contains(id)) {
+      state = "blocked";
+    } else if (registry->terminated_extensions().Contains(id)) {
+      state = "terminated";
+    } else {
+      state = "not_installed";
+    }
+
+    properties.Set("state", state);
+
+    browseros_metrics::BrowserOSMetrics::Log("ota.extension.unexpected_state",
+                                             std::move(properties));
+
+    LOG(WARNING) << "windrise: Extension " << id << " in state: " << state
+                 << " (context: " << context << ")";
+  }
+}
+
+}  // namespace browseros
