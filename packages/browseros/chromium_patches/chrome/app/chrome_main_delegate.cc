diff --git a/chrome/app/chrome_main_delegate.cc b/chrome/app/chrome_main_delegate.cc
--- a/chrome/app/chrome_main_delegate.cc
+++ b/chrome/app/chrome_main_delegate.cc
@@ -303,6 +303,10 @@ std::optional<int> MaybeAutoDeElevate(const base::CommandLine& command_line) {
       // Do not interfere with automation scenarios, which might want to launch
       // Chrome elevated.
       switches::kEnableAutomation,
+      // Do not de-elevate when launched with --remote-debugging-pipe: the
+      // host (e.g. Windrise Bridge) communicates over inherited pipe FDs 3/4,
+      // which base::win::RunDeElevated() via ShellExecute cannot preserve.
+      switches::kRemoteDebuggingPipe,
       // Never attempt to de-elevate a second time.
       switches::kDoNotDeElevateOnLaunch,
       // Do not de-elevate in an isolated child browser process. If de-elevation
