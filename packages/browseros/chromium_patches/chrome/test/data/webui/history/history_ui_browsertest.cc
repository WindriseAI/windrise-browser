diff --git a/chrome/test/data/webui/history/history_ui_browsertest.cc b/chrome/test/data/webui/history/history_ui_browsertest.cc
index 80efd6eae3..293daba4a0 100644
--- a/chrome/test/data/webui/history/history_ui_browsertest.cc
+++ b/chrome/test/data/webui/history/history_ui_browsertest.cc
@@ -73,13 +73,7 @@ IN_PROC_BROWSER_TEST_F(HistoryTest, RoutingWithQueryParam) {
 }
 
 IN_PROC_BROWSER_TEST_F(HistoryTest, SyncedTabs) {
-#if BUILDFLAG(IS_ANDROID)
-  // <history-sync-optin> tests Desktop DICE web-only sign-in promo flows.
-  RunTest("history/history_synced_tabs_test.js",
-          "runMochaSuite('<history-synced-device-manager>')");
-#else
   RunTest("history/history_synced_tabs_test.js", "mocha.run()");
-#endif
 }
 
 IN_PROC_BROWSER_TEST_F(HistoryTest, Toolbar) {
@@ -320,13 +314,3 @@ IN_PROC_BROWSER_TEST_F(HistoryWithHistoryEmbeddingsTest, HistoryAppTest) {
   RunTest("history/history_app_test.js", "runMochaSuite('HistoryAppTest')");
 }
 
-// HistoryAppUnoPhase2FollowUpTest is only available outside CrOS and Android.
-// Android does not participate in the Desktop Uno identity and sync promo flows
-// (kUnoPhase2FollowUp is desktop-only DICE), matching CrOS.
-#if !BUILDFLAG(IS_CHROMEOS) && !BUILDFLAG(IS_ANDROID)
-IN_PROC_BROWSER_TEST_F(HistoryWithHistoryEmbeddingsTest,
-                       HistoryAppUnoPhase2FollowUpTest) {
-  RunTest("history/history_app_test.js",
-          "runMochaSuite('HistoryAppUnoPhase2FollowUpTest')");
-}
-#endif  // !BUILDFLAG(IS_CHROMEOS) && !BUILDFLAG(IS_ANDROID)
