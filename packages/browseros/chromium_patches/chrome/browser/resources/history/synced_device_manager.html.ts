diff --git a/chrome/browser/resources/history/synced_device_manager.html.ts b/chrome/browser/resources/history/synced_device_manager.html.ts
index b4d4a81b76..9cb8136990 100644
--- a/chrome/browser/resources/history/synced_device_manager.html.ts
+++ b/chrome/browser/resources/history/synced_device_manager.html.ts
@@ -4,9 +4,6 @@
 
 import {html} from '//resources/lit/v3_0/lit.rollup.js';
 
-// <if expr="not is_chromeos">
-import {HistorySignInState} from './constants.js';
-// </if>
 
 import type {HistorySyncedDeviceManagerElement} from './synced_device_manager.js';
 
@@ -31,116 +28,6 @@ export function getHtml(this: HistorySyncedDeviceManagerElement) {
   ${this.noSyncedTabsMessage_()}
 </div>
 
-<div id="signInGuide"
-    ?hidden="${
-        !this.showSignInGuide_() || this.replaceSyncPromosWithSignInPromos_}">
-  <div id="sync-promo-illustration"></div>
-  <div id="turn-on-sync-promo">$i18n{turnOnSyncPromo}</div>
-  <div id="turn-on-sync-promo-desc">$i18n{turnOnSyncPromoDesc}</div>
-  <cr-button id="turn-on-sync-button" class="action-button"
-      @click="${this.onTurnOnSyncClick_}">
-    $i18n{turnOnSyncButton}
-  </cr-button>
-</div>
-
-<if expr="not is_chromeos">
-${this.shouldShowHistorySyncOptIn_() ? html`
-  <div id="history-sync-optin" class="history-sync-optin">
-    ${this.isSignInState_(HistorySignInState.SIGNED_OUT) ||
-            this.isSignInState_(HistorySignInState.WEB_ONLY_SIGNED_IN) ?
-        html`
-      <div class="image-container">
-        <img class="sync-history-illustration" alt="">
-      </div>
-      <h1 class="sync-history-promo">$i18n{turnOnSyncHistoryPromo}</h1>
-      ${this.isSignInState_(HistorySignInState.SIGNED_OUT) ? html`
-        <div id="signed-out-sync-history-promo-desc"
-            class="sync-history-promo-desc">
-          $i18n{syncHistoryPromoBodySignedOut}
-        </div>
-      ` : ''}
-      ${this.isSignInState_(HistorySignInState.WEB_ONLY_SIGNED_IN) ? html`
-        <div id="web-only-signed-in-sync-history-promo-desc"
-            class="sync-history-promo-desc">
-          $i18n{syncHistoryPromoBodyWebOnlySignedIn}
-        </div>
-      ` : ''}
-    ` : ''}
-
-    ${this.isSignInState_(HistorySignInState.WEB_ONLY_SIGNED_IN) &&
-            this.accountInfo_ ?
-        html`
-      <div class="profile-row">
-        <img id="profile-icon" class="profile-icon"
-            src="${this.accountInfo_.accountImageSrc}">
-        <div class="account-info-container">
-          <div id="account-name" class="account-name">
-            ${this.accountInfo_.name}
-          </div>
-          <div id="account-email" class="account-email">
-            ${this.accountInfo_.email}
-          </div>
-        </div>
-      </div>
-    ` : ''}
-
-    ${this.isSignInState_(HistorySignInState.SIGN_IN_PENDING) &&
-            this.accountInfo_ ?
-        html`
-      <div class="image-container">
-        <img class="sync-history-promo-avatar-illustration" alt="">
-        <img id="sign-in-pending-avatar" class="avatar"
-            src="${this.accountInfo_.accountImageSrc}" alt="">
-      </div>
-      <h1 class="sync-history-promo">$i18n{turnOnSyncHistoryPromo}</h1>
-
-      ${!this.isTabsSyncTurnedOn_() ? html`
-        <div id="sign-in-pending-sync-history-promo-desc"
-            class="sync-history-promo-desc">
-          $i18n{syncHistoryPromoBodyPendingSignIn}
-        </div>
-      ` : ''}
-
-      ${this.isTabsSyncTurnedOn_() ? html`
-        <div id="sign-in-pending-sync-history-promo-desc-sync-history-on"
-            class="sync-history-promo-desc">
-          $i18n{syncHistoryPromoBodyPendingSignInSyncHistoryOn}
-        </div>
-      ` : ''}
-    ` : ''}
-
-    ${this.isSignInState_(HistorySignInState.SIGNED_IN) &&
-            !this.isTabsSyncTurnedOn_() && this.accountInfo_ ?
-        html`
-      <div class="image-container">
-        <img class="sync-history-promo-avatar-illustration" alt="">
-        <img id="signed-in-avatar" class="avatar"
-            src="${this.accountInfo_.accountImageSrc}" alt="">
-      </div>
-      <h1 class="sync-history-promo">$i18n{turnOnSyncHistoryPromo}</h1>
-      <div id="signed-in-sync-history-promo-desc"
-          class="sync-history-promo-desc">
-        $i18n{turnOnSignedInSyncHistoryPromoBodySignInSyncOff}
-      </div>
-    ` : ''}
-
-    <!-- Button -->
-    ${this.isSignInState_(HistorySignInState.SIGN_IN_PENDING) &&
-            this.isTabsSyncTurnedOn_() ?
-        html`
-      <cr-button id="verify-its-you-button" class="action-button"
-          @click="${this.onTurnOnHistorySyncClick_}">
-        $i18n{verifyItsYou}
-      </cr-button>
-    ` : html`
-      <cr-button id="sync-history-button" class="action-button"
-          @click="${this.onTurnOnHistorySyncClick_}">
-        $i18n{turnOnSyncHistoryButton}
-      </cr-button>
-    `}
-  </div>
-` : ''}
-</if>
 
 <cr-lazy-render-lit id="menu"
     .template="${() => html`
