diff --git a/chrome/browser/resources/history/app.html.ts b/chrome/browser/resources/history/app.html.ts
index 273d97db78..dd02557b75 100644
--- a/chrome/browser/resources/history/app.html.ts
+++ b/chrome/browser/resources/history/app.html.ts
@@ -83,20 +83,6 @@ export function getHtml(this: HistoryAppElement) {
       ` : ''}
       <div id="tabsScrollContainer" class="cr-scrollable">
         <div class="cr-scrollable-top-shadow" ?hidden="${this.showTabs_}"></div>
-<if expr="not is_chromeos">
-        ${this.shouldShowHistoryPageHistorySyncPromo_() ? html`
-          <div class="history-cards">
-            <history-sync-promo></history-sync-promo>
-          </div>
-        ` : ''}
-        <div class="history-cards"
-            ?hidden="${!this.shouldShowHistoryCrossDeviceSigninPromo_}">
-          <history-cross-device-signin-promo id="historyCrossDeviceSigninPromo"
-              @should-show-history-cross-device-signin-promo="${
-                  this.onShouldShowHistoryCrossDeviceSigninPromo_}">
-          </history-cross-device-signin-promo>
-        </div>
-</if>
         ${this.enableHistoryEmbeddings_ ? html`
           <div id="historyEmbeddingsContainer" class="history-cards">
             <history-embeddings-promo></history-embeddings-promo>
