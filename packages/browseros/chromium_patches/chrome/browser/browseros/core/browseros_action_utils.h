diff --git a/chrome/browser/browseros/core/browseros_action_utils.h b/chrome/browser/browseros/core/browseros_action_utils.h
new file mode 100644
index 0000000000000..cf8770fd34479
--- /dev/null
+++ b/chrome/browser/browseros/core/browseros_action_utils.h
@@ -0,0 +1,40 @@
+// Copyright 2025 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_ACTION_UTILS_H_
+#define CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_ACTION_UTILS_H_
+
+#include <string>
+#include <string_view>
+
+#include "chrome/browser/browseros/core/browseros_constants.h"
+#include "chrome/browser/ui/actions/chrome_action_id.h"
+#include "chrome/browser/ui/side_panel/side_panel_entry_key.h"
+#include "chrome/common/chrome_features.h"
+#include "ui/actions/actions.h"
+
+namespace browseros {
+
+inline bool IsBrowserOSAction(actions::ActionId id) {
+  if (id == kActionBrowserOSAgent) {
+    return browseros::IsActiveBrowserOSExtension(browseros::kAgentExtensionId);
+  }
+
+  for (const auto& ext_id : browseros::GetActiveBrowserOSExtensionIds()) {
+    if (!browseros::IsBrowserOSLabelledExtension(ext_id)) {
+      continue;
+    }
+    auto ext_action_id = actions::ActionIdMap::StringToActionId(
+        SidePanelEntryKey(SidePanelEntryId::kExtension, ext_id).ToString());
+    if (ext_action_id && id == *ext_action_id) {
+      return true;
+    }
+  }
+
+  return false;
+}
+
+}  // namespace browseros
+
+#endif  // CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_ACTION_UTILS_H_
