diff --git a/chrome/browser/browseros/server/server_process_launcher.cc b/chrome/browser/browseros/server/server_process_launcher.cc
new file mode 100644
index 0000000000000000000000000000000000000000..ff8a03f832a4d2692a08d6ee1c468bea01369d56
--- /dev/null
+++ b/chrome/browser/browseros/server/server_process_launcher.cc
@@ -0,0 +1,18 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/server/server_process_launcher.h"
+
+#include "base/process/launch.h"
+
+namespace browseros {
+
+base::Process LaunchServerProcess(const base::CommandLine& command_line,
+                                  const base::EnvironmentMap& environment) {
+  base::LaunchOptions options;
+  options.environment = environment;
+  return base::LaunchProcess(command_line, options);
+}
+
+}  // namespace browseros
