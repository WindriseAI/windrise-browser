diff --git a/chrome/browser/resources/history/side_bar.ts b/chrome/browser/resources/history/side_bar.ts
index f1667adc56..13f9b4f874 100644
--- a/chrome/browser/resources/history/side_bar.ts
+++ b/chrome/browser/resources/history/side_bar.ts
@@ -31,7 +31,6 @@ export interface HistorySideBarElement {
   $: {
     history: HTMLAnchorElement,
     menu: CrMenuSelectorElement,
-    syncedTabs: HTMLElement,
   };
 }
 
