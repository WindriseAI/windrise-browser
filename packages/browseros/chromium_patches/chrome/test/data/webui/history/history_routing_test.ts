diff --git a/chrome/test/data/webui/history/history_routing_test.ts b/chrome/test/data/webui/history/history_routing_test.ts
index 1a9bc4ffad..f2026ef324 100644
--- a/chrome/test/data/webui/history/history_routing_test.ts
+++ b/chrome/test/data/webui/history/history_routing_test.ts
@@ -8,9 +8,8 @@ import type {HistoryAppElement, HistorySideBarElement} from 'chrome://history/hi
 import {BrowserProxyImpl, CrRouter, historyClustersBrowserProxyFactory, historyEmbeddingsBrowserProxyFactory, HistoryEmbeddingsPageHandlerRemote, MetricsProxyImpl, PageHandlerRemote as HistoryClustersPageHandlerRemote} from 'chrome://history/history.js';
 import {loadTimeData} from 'chrome://resources/js/load_time_data.js';
 import {assertEquals, assertTrue} from 'chrome://webui-test/chai_assert.js';
-import {keyDownOn} from 'chrome://webui-test/keyboard_mock_interactions.js';
 import {TestMock} from 'chrome://webui-test/test_mock.js';
-import {eventToPromise, isVisible, microtasksFinished} from 'chrome://webui-test/test_util.js';
+import {isVisible, microtasksFinished} from 'chrome://webui-test/test_util.js';
 
 import {TestMetricsProxy} from './history_clusters/utils.js';
 import {TestHistoryBrowserProxy} from './test_browser_proxy.js';
@@ -57,19 +56,13 @@ import {navigateTo} from './test_util.js';
       return microtasksFinished();
     });
 
-    test('changing route changes active view', async () => {
-      assertEquals('history', app.$.content.selected);
-      assertEquals(app.$.history, app.$.tabsContent.selectedItem);
-
+    test('legacy synced tabs route opens ordinary history', async () => {
       navigateTo('/syncedTabs', app);
-      await eventToPromise('iron-select', sidebar.$.menu);
-
-      assertEquals('chrome://history/syncedTabs', window.location.href);
       await microtasksFinished();
-      assertEquals('syncedTabs', app.$.content.selected);
-      assertEquals(
-          app.shadowRoot.querySelector('#syncedDevicesScroll'),
-          app.$.content.selectedItem);
+
+      assertEquals('chrome://history/', window.location.href);
+      assertEquals('history', app.$.content.selected);
+      assertEquals(app.$.history, app.$.tabsContent.selectedItem);
     });
 
 
@@ -113,19 +106,6 @@ import {navigateTo} from './test_util.js';
       assertEquals('history', sidebar.$.menu.selected);
       assertEquals('chrome://history/', window.location.href);
 
-      sidebar.$.syncedTabs.click();
-      await eventToPromise('iron-select', sidebar.$.menu);
-      await microtasksFinished();
-      assertEquals('syncedTabs', sidebar.$.menu.selected);
-      assertEquals('chrome://history/syncedTabs', window.location.href);
-
-      // Currently selected history view is preserved in sidebar menu item.
-      keyDownOn(sidebar.$.history, 0, [], ' ');
-      await eventToPromise('iron-select', sidebar.$.menu);
-      await microtasksFinished();
-      assertEquals('history', sidebar.$.menu.selected);
-      assertEquals('chrome://history/', window.location.href);
-
       const historyTabs = app.shadowRoot.querySelector('cr-tabs');
       assertEquals(!!historyTabs, isHistoryClustersEnabled);
 
@@ -136,19 +116,6 @@ import {navigateTo} from './test_util.js';
         assertEquals('grouped', sidebar.$.menu.selected);
         assertEquals('chrome://history/grouped', window.location.href);
 
-        keyDownOn(sidebar.$.syncedTabs, 0, [], ' ');
-        await eventToPromise('iron-select', sidebar.$.menu);
-        await microtasksFinished();
-        assertEquals('syncedTabs', sidebar.$.menu.selected);
-        assertEquals('chrome://history/syncedTabs', window.location.href);
-
-        // Currently selected history view is preserved in sidebar menu item.
-        keyDownOn(sidebar.$.history, 0, [], ' ');
-        await eventToPromise('iron-select', sidebar.$.menu);
-        await microtasksFinished();
-        assertEquals('grouped', sidebar.$.menu.selected);
-        assertEquals('chrome://history/grouped', window.location.href);
-
         historyTabs.selected = 0;
         await microtasksFinished();
         assertEquals('history', sidebar.$.menu.selected);
@@ -173,56 +140,28 @@ import {navigateTo} from './test_util.js';
       assertEquals('chrome://history/?q=' + searchTerm, window.location.href);
     });
 
-    test(
-        'search is preserved across tabs and sidebar menu items',
-        async function() {
-          const searchTerm = 'Soldier76';
-          assertEquals('history', sidebar.$.menu.selected);
-          navigateTo('/?q=' + searchTerm, app);
-
-          sidebar.$.syncedTabs.click();
-          await eventToPromise('iron-select', sidebar.$.menu);
-          await microtasksFinished();
-          assertEquals('syncedTabs', sidebar.$.menu.selected);
-          assertEquals(searchTerm, app.$.toolbar.searchTerm);
-          assertEquals(
-              'chrome://history/syncedTabs?q=' + searchTerm,
-              window.location.href);
-
-          sidebar.$.history.click();
-          await eventToPromise('iron-select', sidebar.$.menu);
-          await microtasksFinished();
-          assertEquals('history', sidebar.$.menu.selected);
-          assertEquals(searchTerm, app.$.toolbar.searchTerm);
-          assertEquals(
-              'chrome://history/?q=' + searchTerm, window.location.href);
-
-          if (isHistoryClustersEnabled) {
-            const tabs = app.shadowRoot.querySelector('cr-tabs');
-            assertTrue(!!tabs);
-            tabs.selected = 1;
-            await microtasksFinished();
-            assertEquals('grouped', sidebar.$.menu.selected);
-            assertEquals(searchTerm, app.$.toolbar.searchTerm);
-            assertEquals(
-                'chrome://history/grouped?q=' + searchTerm,
-                window.location.href);
-          }
-        });
-
-    test(
-        'routing to chrome://history/syncedTabs works correctly',
-        async function() {
-          navigateTo('/syncedTabs', app);
-          if (isHistoryClustersEnabled) {
-            // cr-tabs can change their selected value, but these should be
-            // ignored since /syncedTabs is not a tabbed page.
-            const historyTabs = app.shadowRoot.querySelector('cr-tabs')!;
-            historyTabs.selected = -1;
-            await microtasksFinished();
-          }
-          assertEquals('chrome://history/syncedTabs', window.location.href);
-        });
+    test('legacy synced tabs route preserves search', async () => {
+      const searchTerm = 'Soldier76';
+      navigateTo('/syncedTabs?q=' + searchTerm, app);
+      await microtasksFinished();
+
+      assertEquals('history', sidebar.$.menu.selected);
+      assertEquals(searchTerm, app.$.toolbar.searchTerm);
+      assertEquals(
+          'chrome://history/?q=' + searchTerm, window.location.href);
+
+      if (isHistoryClustersEnabled) {
+        const tabs = app.shadowRoot.querySelector('cr-tabs');
+        assertTrue(!!tabs);
+        tabs.selected = 1;
+        await microtasksFinished();
+        assertEquals('grouped', sidebar.$.menu.selected);
+        assertEquals(searchTerm, app.$.toolbar.searchTerm);
+        assertEquals(
+            'chrome://history/grouped?q=' + searchTerm,
+            window.location.href);
+      }
+    });
   });
 });
 
