diff --git a/chrome/browser/resources/history/router.ts b/chrome/browser/resources/history/router.ts
index 25af2e8fe3..a98e6cf5ee 100644
--- a/chrome/browser/resources/history/router.ts
+++ b/chrome/browser/resources/history/router.ts
@@ -8,7 +8,7 @@ import {EventTracker} from 'chrome://resources/js/event_tracker.js';
 import {CrLitElement} from 'chrome://resources/lit/v3_0/lit.rollup.js';
 import type {PropertyValues} from 'chrome://resources/lit/v3_0/lit.rollup.js';
 
-// All valid pages.
+// History pages, including the legacy synced-tabs URL token.
 // TODO(crbug.com/40069898): Change this to an enum and use that type for holding
 //  these values for better type check when `loadTimeData` is no longer needed.
 export const Page = {
@@ -128,6 +128,11 @@ export class HistoryRouterElement extends CrLitElement {
     const page = sections[0] ||
         (window.location.search ? 'history' :
                                   TABBED_PAGES[this.lastSelectedTab]);
+    if (page === Page.SYNCED_TABS) {
+      CrRouter.getInstance().setPath('/');
+      this.selectedPage = Page.HISTORY;
+      return;
+    }
     // TODO(b/338245900): This is kind of nasty. Without cr-tabs to constrain
     //   `selectedPage`, this can be set to an arbitrary value from the URL.
     //   To fix this, we should constrain the selected pages to an actual enum.
