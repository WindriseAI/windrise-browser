diff --git a/chrome/app/chrome_crash_reporter_client_win.cc b/chrome/app/chrome_crash_reporter_client_win.cc
index f7ce95d3977ce5d0dc3193a11083209f151f2a14..eeb01977a7d97de52fd7d59570b52a9954b4114f 100644
--- a/chrome/app/chrome_crash_reporter_client_win.cc
+++ b/chrome/app/chrome_crash_reporter_client_win.cc
@@ -95,9 +95,9 @@ void ChromeCrashReporterClient::GetProductInfo(ProductInfo* product_info) {
   GetProductNameAndVersion(exe_file, &product_name, &version, &special_build,
                            &channel_name);
 
-  *product_info =
-      ProductInfo(base::WideToUTF8(product_name), base::WideToUTF8(version),
-                  base::WideToUTF8(channel_name));
+  *product_info = ProductInfo(base::WideToUTF8(product_name),
+                             base::WideToUTF8(version),
+                             base::WideToUTF8(channel_name));
 }
 
 bool ChromeCrashReporterClient::GetShouldDumpLargerDumps() {
@@ -146,7 +146,8 @@ bool ChromeCrashReporterClient::IsRunningUnattended() {
 }
 
 bool ChromeCrashReporterClient::GetCollectStatsConsent() {
-  return install_static::GetCollectStatsConsent();
+  // Crash upload is disabled by default; minidumps stay local.
+  return false;
 }
 
 bool ChromeCrashReporterClient::GetCollectStatsInSample() {
@@ -236,3 +237,7 @@ std::wstring ChromeCrashReporterClient::GetWerRuntimeExceptionModule() {
   // file_start points to the start of the filename in the elf_dir buffer.
   return std::wstring(elf_dir, file_start).append(kWerDll);
 }
+
+std::string ChromeCrashReporterClient::GetUploadUrl() {
+  return std::string();
+}
