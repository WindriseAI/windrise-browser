diff --git a/chrome/browser/resources/settings/router.ts b/chrome/browser/resources/settings/router.ts
index 47b4a045f3..ff0006b19b 100644
--- a/chrome/browser/resources/settings/router.ts
+++ b/chrome/browser/resources/settings/router.ts
@@ -289,6 +289,11 @@ export class Router {
    */
   setCurrentRoute(
       route: Route, queryParameters: URLSearchParams, isPopstate: boolean) {
+    if (route === this.routes_.SYNC || route === this.routes_.SYNC_ADVANCED) {
+      route = this.routes_.BASIC;
+      queryParameters = new URLSearchParams();
+      window.history.replaceState(undefined, '', route.path);
+    }
     this.recordMetrics(route.path);
 
     const oldRoute = this.currentRoute;
@@ -393,7 +398,8 @@ export class Router {
       removeSearch: boolean = false) {
     // The ADVANCED route only serves as a parent of subpages, and should not
     // be possible to navigate to it directly.
-    if (route === this.routes_.ADVANCED) {
+    if (route === this.routes_.ADVANCED || route === this.routes_.SYNC ||
+        route === this.routes_.SYNC_ADVANCED) {
       route = this.routes_.BASIC;
     }
 
@@ -450,8 +456,9 @@ export class Router {
     // record all incorrect paths as hitting the main settings page.
     this.recordMetrics(route ? route.path : this.routes_.BASIC.path);
 
-    // Never allow direct navigation to ADVANCED.
-    if (route && route !== this.routes_.ADVANCED) {
+    // Windrise does not offer browser sync or direct ADVANCED navigation.
+    if (route && route !== this.routes_.ADVANCED &&
+        route !== this.routes_.SYNC && route !== this.routes_.SYNC_ADVANCED) {
       this.currentRoute = route;
       this.currentQueryParameters_ =
           new URLSearchParams(window.location.search);
