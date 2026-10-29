diff --git a/chrome/browser/browseros/server/server_process_launcher_win.cc b/chrome/browser/browseros/server/server_process_launcher_win.cc
new file mode 100644
index 0000000000000000000000000000000000000000..f6d0bdb166f5ba99abe06a3b8095a0f8d17863cd
--- /dev/null
+++ b/chrome/browser/browseros/server/server_process_launcher_win.cc
@@ -0,0 +1,52 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/server/server_process_launcher.h"
+
+#include <windows.h>
+
+#include <string>
+
+#include "base/command_line.h"
+#include "base/logging.h"
+#include "base/process/environment_internal.h"
+#include "base/threading/scoped_thread_priority.h"
+#include "base/win/scoped_process_information.h"
+#include "base/win/startup_information.h"
+
+namespace browseros {
+
+base::Process LaunchServerProcess(const base::CommandLine& command_line,
+                                  const base::EnvironmentMap& environment) {
+  base::win::StartupInformation startup_info;
+  STARTUPINFO* startup_info_ptr = startup_info.startup_info();
+  startup_info_ptr->dwFlags |= STARTF_USESHOWWINDOW;
+  startup_info_ptr->wShowWindow = SW_HIDE;
+
+  PROCESS_INFORMATION raw_process_info = {};
+  std::wstring writable_command_line = command_line.GetCommandLineString();
+
+  DWORD flags = CREATE_NO_WINDOW;
+  std::wstring environment_block;
+  wchar_t* environment_ptr = nullptr;
+  if (!environment.empty()) {
+    environment_block = base::internal::AlterEnvironment(
+        base::internal::GetEnvironment(), environment);
+    environment_ptr = environment_block.data();
+    flags |= CREATE_UNICODE_ENVIRONMENT;
+  }
+
+  SCOPED_MAY_LOAD_LIBRARY_AT_BACKGROUND_PRIORITY();
+  if (!::CreateProcessW(nullptr, writable_command_line.data(), nullptr, nullptr,
+                        FALSE, flags, environment_ptr, nullptr,
+                        startup_info_ptr, &raw_process_info)) {
+    DPLOG(ERROR) << "browseros: Failed to launch managed server";
+    return base::Process();
+  }
+
+  base::win::ScopedProcessInformation process_info(raw_process_info);
+  return base::Process(process_info.TakeProcessHandle());
+}
+
+}  // namespace browseros
