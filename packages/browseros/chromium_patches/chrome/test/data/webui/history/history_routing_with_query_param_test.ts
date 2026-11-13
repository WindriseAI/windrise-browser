diff --git a/chrome/test/data/webui/history/history_routing_with_query_param_test.ts b/chrome/test/data/webui/history/history_routing_with_query_param_test.ts
index bc65da8642..1fab5da119 100644
--- a/chrome/test/data/webui/history/history_routing_with_query_param_test.ts
+++ b/chrome/test/data/webui/history/history_routing_with_query_param_test.ts
@@ -54,6 +54,20 @@ suite('routing-with-query-param', function() {
         expectedQuery, app.$.toolbar.$.mainToolbar.getSearchField().getValue());
   });
 
+  test('legacy synced tabs keeps search and date filters', async () => {
+    await testProxy.handler.whenCalled('queryHistory');
+    loadTimeData.overrideValues({enableHistoryEmbeddings: true});
+
+    navigateTo('/syncedTabs?q=query&after=2011-04-05', app);
+    await microtasksFinished();
+
+    assertEquals(
+        'chrome://history/?q=query&after=2011-04-05', window.location.href);
+    assertEquals('history', app.$.content.selected);
+    assertEquals(
+        expectedQuery, app.$.toolbar.$.mainToolbar.getSearchField().getValue());
+  });
+
   test('search with after date', async () => {
     // Wait for initial query to get called.
     await testProxy.handler.whenCalled('queryHistory');
