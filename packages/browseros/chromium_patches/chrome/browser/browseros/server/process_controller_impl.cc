diff --git a/chrome/browser/browseros/server/process_controller_impl.cc b/chrome/browser/browseros/server/process_controller_impl.cc
new file mode 100644
index 0000000000000000000000000000000000000000..a621f70d3c24eaad319a90fc999d2b2acb4c77e8
--- /dev/null
+++ b/chrome/browser/browseros/server/process_controller_impl.cc
@@ -0,0 +1,172 @@
+// Copyright 2024 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/server/process_controller_impl.h"
+
+#include <optional>
+
+#include "base/files/file_util.h"
+#include "base/json/json_writer.h"
+#include "base/logging.h"
+#include "build/build_config.h"
+#include "chrome/browser/browseros/server/browseros_server_utils.h"
+#include "chrome/browser/browseros/server/server_process_launcher.h"
+
+#if BUILDFLAG(IS_POSIX)
+#include <signal.h>
+#endif
+
+namespace browseros {
+
+namespace {
+
+base::FilePath WriteConfigJson(const ServerLaunchConfig& config,
+                               const base::FilePath& actual_resources_dir) {
+  base::FilePath config_path =
+      config.paths.execution.Append(config.config_file_name);
+
+  base::DictValue root = BuildServerConfigJson(config, actual_resources_dir);
+
+  std::optional<std::string> json_output = base::WriteJson(root);
+  if (!json_output.has_value()) {
+    LOG(ERROR) << "windrise: Failed to serialize " << config.log_name
+               << " config to JSON";
+    return base::FilePath();
+  }
+
+  if (!base::WriteFile(config_path, json_output.value())) {
+    LOG(ERROR) << "windrise: Failed to write " << config.log_name
+               << " config file: " << config_path;
+    return base::FilePath();
+  }
+
+  LOG(INFO) << "windrise: Wrote " << config.log_name << " config to "
+            << config_path;
+  return config_path;
+}
+
+}  // namespace
+
+ProcessControllerImpl::ProcessControllerImpl() = default;
+
+ProcessControllerImpl::~ProcessControllerImpl() = default;
+
+LaunchResult ProcessControllerImpl::Launch(const ServerLaunchConfig& config) {
+  LaunchResult result;
+  base::FilePath actual_exe_path = config.paths.exe;
+  base::FilePath actual_resources_dir = config.paths.resources;
+
+  // Check if executable exists, fallback to bundled if not
+  if (!base::PathExists(actual_exe_path)) {
+    LOG(WARNING) << "windrise: Binary not found at " << actual_exe_path
+                 << ", falling back to bundled";
+    actual_exe_path = config.paths.fallback_exe;
+    actual_resources_dir = config.paths.fallback_resources;
+    result.used_fallback = true;
+
+    if (!base::PathExists(actual_exe_path)) {
+      LOG(ERROR) << "windrise: Bundled binary also not found at: "
+                 << actual_exe_path;
+      return result;
+    }
+  }
+
+  if (config.paths.execution.empty()) {
+    LOG(ERROR) << "windrise: Execution directory path is empty";
+    return result;
+  }
+
+  // Ensure execution directory exists (blocking I/O)
+  if (!base::CreateDirectory(config.paths.execution)) {
+    LOG(ERROR) << "windrise: Failed to create execution directory at: "
+               << config.paths.execution;
+    return result;
+  }
+
+  base::FilePath config_path = WriteConfigJson(config, actual_resources_dir);
+  if (config_path.empty()) {
+    LOG(ERROR) << "windrise: Failed to write config file, aborting launch";
+    return result;
+  }
+
+  base::CommandLine cmd(actual_exe_path);
+  cmd.AppendSwitchPath("config", config_path);
+
+  // Windrise: pin the state directory to this profile's execution directory.
+  base::EnvironmentMap environment;
+  environment[FILE_PATH_LITERAL("WINDRISE_BROWSER_DIR")] =
+      config.paths.execution.value();
+
+  result.process = LaunchServerProcess(cmd, environment);
+  return result;
+}
+
+void ProcessControllerImpl::Terminate(base::Process* process, bool wait) {
+  if (!process || !process->IsValid()) {
+    return;
+  }
+
+  LOG(INFO) << "windrise: Terminating process with SIGKILL (PID: "
+            << process->Pid() << ", wait: " << (wait ? "true" : "false") << ")";
+
+#if BUILDFLAG(IS_POSIX)
+  base::ProcessId pid = process->Pid();
+  if (kill(pid, SIGKILL) != 0) {
+    PLOG(ERROR) << "windrise: Failed to send SIGKILL to PID " << pid;
+  } else if (wait) {
+    // Blocking wait - caller must ensure this runs on a thread with MayBlock()
+    int exit_code = 0;
+    if (process->WaitForExit(&exit_code)) {
+      LOG(INFO) << "windrise: Process killed successfully";
+    } else {
+      LOG(WARNING) << "windrise: WaitForExit failed";
+    }
+  } else {
+    LOG(INFO) << "windrise: SIGKILL sent (not waiting for exit)";
+  }
+#else
+  // Windows: Terminate with wait parameter
+  bool terminated = process->Terminate(0, wait);
+  if (terminated) {
+    LOG(INFO) << "windrise: Process terminated successfully";
+  } else {
+    LOG(ERROR) << "windrise: Failed to terminate process";
+  }
+#endif
+}
+
+bool ProcessControllerImpl::WaitForExitWithTimeout(base::Process* process,
+                                                   base::TimeDelta timeout,
+                                                   int* exit_code) {
+  if (!process || !process->IsValid()) {
+    return true;  // No process to wait for
+  }
+
+  LOG(INFO) << "windrise: Waiting for process exit (PID: " << process->Pid()
+            << ", timeout: " << timeout.InSeconds() << "s)";
+
+  bool exited = process->WaitForExitWithTimeout(timeout, exit_code);
+  if (exited) {
+    LOG(INFO) << "windrise: Process exited with code " << *exit_code;
+  } else {
+    LOG(INFO) << "windrise: Process did not exit within timeout";
+  }
+  return exited;
+}
+
+bool ProcessControllerImpl::Exists(base::ProcessId pid) {
+  return server_utils::ProcessExists(pid);
+}
+
+std::optional<int64_t> ProcessControllerImpl::GetCreationTime(
+    base::ProcessId pid) {
+  return server_utils::GetProcessCreationTime(pid);
+}
+
+bool ProcessControllerImpl::Kill(base::ProcessId pid,
+                                 base::TimeDelta graceful_timeout) {
+  return server_utils::KillProcess(pid, graceful_timeout);
+}
+
+}  // namespace browseros
