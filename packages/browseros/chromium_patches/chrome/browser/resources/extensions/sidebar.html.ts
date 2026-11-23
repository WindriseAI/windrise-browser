diff --git a/chrome/browser/resources/extensions/sidebar.html.ts b/chrome/browser/resources/extensions/sidebar.html.ts
index 095bbc0..446865b 100644
--- a/chrome/browser/resources/extensions/sidebar.html.ts
+++ b/chrome/browser/resources/extensions/sidebar.html.ts
@@ -53,8 +53,8 @@ ${this.showDeveloperPromo_() ? html`
     </span>
   </div>
 ` : ''}
-<div class="separator"></div>
-<div class="cr-nav-menu-item" id="moreExtensions">
+<div class="separator" hidden></div>
+<div class="cr-nav-menu-item" id="moreExtensions" hidden>
   <cr-icon id="web-store-icon" icon="extensions-icons:web_store-custom">
   </cr-icon>
   <span id="discover-more-text" class="cr-secondary-text"
