diff --git a/chrome/browser/browseros/server/server_process_launcher.h b/chrome/browser/browseros/server/server_process_launcher.h
new file mode 100644
index 0000000000000000000000000000000000000000..cf3fa5b68f8349db467dfb6067b5fab0313f8014
--- /dev/null
+++ b/chrome/browser/browseros/server/server_process_launcher.h
@@ -0,0 +1,26 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_BROWSEROS_SERVER_SERVER_PROCESS_LAUNCHER_H_
+#define CHROME_BROWSER_BROWSEROS_SERVER_SERVER_PROCESS_LAUNCHER_H_
+
+#include "base/environment.h"
+#include "base/process/process.h"
+
+namespace base {
+class CommandLine;
+}
+
+namespace browseros {
+
+// Launches a managed server with platform-appropriate background semantics.
+// `environment` is applied on top of the browser's own environment (see
+// base::LaunchOptions::environment); an empty map inherits it unchanged.
+base::Process LaunchServerProcess(
+    const base::CommandLine& command_line,
+    const base::EnvironmentMap& environment = {});
+
+}  // namespace browseros
+
+#endif  // CHROME_BROWSER_BROWSEROS_SERVER_SERVER_PROCESS_LAUNCHER_H_
