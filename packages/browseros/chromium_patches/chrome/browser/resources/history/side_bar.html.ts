diff --git a/chrome/browser/resources/history/side_bar.html.ts b/chrome/browser/resources/history/side_bar.html.ts
index f2384bf..2d301c2 100644
--- a/chrome/browser/resources/history/side_bar.html.ts
+++ b/chrome/browser/resources/history/side_bar.html.ts
@@ -20,13 +20,8 @@ export function getHtml(this: HistorySideBarElement) {
     $i18n{historyMenuItem}
     <cr-ripple></cr-ripple>
   </a>
-  <a id="syncedTabs" role="menuitem" href="/syncedTabs"
-      class="page-item cr-nav-menu-item" path="syncedTabs"
-      @click="${this.onItemClick_}">
-    <cr-icon icon="cr:devices"></cr-icon>
-    $i18n{openTabsMenuItem}
-    <cr-ripple></cr-ripple>
-  </a>
+  <!-- Windrise: "Tabs from other devices" needs sync, which this build
+       cannot use, so the sidebar entry is removed. -->
   <a role="menuitem" id="clear-browsing-data" class="cr-nav-menu-item"
       href="chrome://settings/clearBrowserData"
       @click="${this.onClearBrowsingDataClick_}"
