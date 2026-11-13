diff --git a/chrome/browser/resources/history/synced_device_manager.ts b/chrome/browser/resources/history/synced_device_manager.ts
index 90a83fd9ac..f459392f3d 100644
--- a/chrome/browser/resources/history/synced_device_manager.ts
+++ b/chrome/browser/resources/history/synced_device_manager.ts
@@ -3,17 +3,12 @@
 // found in the LICENSE file.
 
 import 'chrome://resources/cr_elements/cr_action_menu/cr_action_menu.js';
-import 'chrome://resources/cr_elements/cr_button/cr_button.js';
 import 'chrome://resources/cr_elements/cr_lazy_render/cr_lazy_render_lit.js';
 import './synced_device_card.js';
 import '/strings.m.js';
 
 import {browserProxyFactory} from 'chrome://resources/cr_components/history/foreign_sessions.mojom-webui.js';
 import type {ForeignSession, ForeignSessionTab} from 'chrome://resources/cr_components/history/foreign_sessions.mojom-webui.js';
-import {AccessPoint} from 'chrome://resources/cr_components/history/history.mojom-webui.js';
-// <if expr="not is_chromeos">
-import type {AccountInfo} from 'chrome://resources/cr_components/history/history.mojom-webui.js';
-// </if>
 import type {CrActionMenuElement} from 'chrome://resources/cr_elements/cr_action_menu/cr_action_menu.js';
 import type {CrLazyRenderLitElement} from 'chrome://resources/cr_elements/cr_lazy_render/cr_lazy_render_lit.js';
 import {WebUiListenerMixinLit} from 'chrome://resources/cr_elements/web_ui_listener_mixin_lit.js';
@@ -52,7 +47,6 @@ export interface HistorySyncedDeviceManagerElement {
   $: {
     menu: CrLazyRenderLitElement<CrActionMenuElement>,
     noSyncedTabs: HTMLElement,
-    signInGuide: HTMLElement,
   };
 }
 
@@ -83,7 +77,6 @@ export class HistorySyncedDeviceManagerElement extends
       syncedDevices_: {type: Array},
       historyIdentityState_: {type: Object},
       guestSession_: {type: Boolean},
-      signInAllowed_: {type: Boolean},
       fetchingSyncedTabs_: {type: Boolean},
       hasSeenForeignData_: {type: Boolean},
       /**
@@ -93,9 +86,6 @@ export class HistorySyncedDeviceManagerElement extends
 
       replaceSyncPromosWithSignInPromos_: {type: Boolean},
 
-      // <if expr="not is_chromeos">
-      accountInfo_: {type: Object},
-      // </if>
     };
   }
 
@@ -107,15 +97,8 @@ export class HistorySyncedDeviceManagerElement extends
   private accessor actionMenuModel_: string|null = null;
   private accessor guestSession_: boolean =
       loadTimeData.getBoolean('isGuestSession');
-  private accessor signInAllowed_: boolean =
-      loadTimeData.getBoolean('isSignInAllowed');
   protected accessor replaceSyncPromosWithSignInPromos_: boolean =
       loadTimeData.getBoolean('replaceSyncPromosWithSignInPromos');
-  private signinPausedImpressionRecorded_: boolean = false;
-  // <if expr="not is_chromeos">
-  protected accessor accountInfo_: AccountInfo|null = null;
-  private onAccountInfoDataReceivedListenerId_: number|null = null;
-  // </if>
 
   private accessor historyIdentityState_: HistoryIdentityState = {
     signIn: HistorySignInState.SIGNED_OUT,
@@ -145,28 +128,12 @@ export class HistorySyncedDeviceManagerElement extends
         (identityState: HistoryIdentityState) => this.historyIdentityState_ =
             identityState);
 
-    // <if expr="not is_chromeos">
-    this.onAccountInfoDataReceivedListenerId_ =
-        BrowserProxyImpl.getInstance()
-            .callbackRouter.sendAccountInfo.addListener(
-                this.handleAccountInfoChanged_.bind(this));
-
-    BrowserProxyImpl.getInstance().handler.requestAccountInfo().then(
-        ({accountInfo}) => this.handleAccountInfoChanged_(accountInfo));
-    // </if>
   }
 
   override disconnectedCallback() {
     super.disconnectedCallback();
     this.focusGrid_!.destroy();
 
-    // <if expr="not is_chromeos">
-    if (this.onAccountInfoDataReceivedListenerId_ !== null) {
-      BrowserProxyImpl.getInstance().callbackRouter.removeListener(
-          this.onAccountInfoDataReceivedListenerId_);
-      this.onAccountInfoDataReceivedListenerId_ = null;
-    }
-    // </if>
   }
 
   override willUpdate(changedProperties: PropertyValues<this>) {
@@ -194,14 +161,6 @@ export class HistorySyncedDeviceManagerElement extends
     this.addEventListener('update-focus-grid', this.updateFocusGrid_);
   }
 
-  configureSignInForTest(data: {
-    signInAllowed: boolean,
-    guestSession: boolean,
-  }) {
-    this.signInAllowed_ = data.signInAllowed;
-    this.guestSession_ = data.guestSession;
-  }
-
   private createInternalDevice_(session: ForeignSession):
       ForeignDeviceInternal {
     let tabs: ForeignSessionTab[] = [];
@@ -247,20 +206,6 @@ export class HistorySyncedDeviceManagerElement extends
     };
   }
 
-  protected onTurnOnSyncClick_() {
-    BrowserProxyImpl.getInstance().startTurnOnSyncFlow();
-  }
-
-  // <if expr="not is_chromeos">
-  protected onTurnOnHistorySyncClick_() {
-    BrowserProxyImpl.getInstance().handler.turnOnHistorySync(
-        AccessPoint.kRecentTabs);
-  }
-
-  private handleAccountInfoChanged_(accountInfo: AccountInfo) {
-    this.accountInfo_ = accountInfo;
-  }
-  // </if>
 
   private onOpenMenu_(e: CustomEvent<{tag: string, target: HTMLElement}>) {
     this.actionMenuModel_ = e.detail.tag;
@@ -335,12 +280,6 @@ export class HistorySyncedDeviceManagerElement extends
     return this.historyIdentityState_.signIn === state;
   }
 
-  protected shouldShowHistorySyncOptIn_(): boolean {
-    return this.replaceSyncPromosWithSignInPromos_ &&
-        !this.isTabsSyncDisabled_() &&
-        !(this.isSignInState_(HistorySignInState.SIGNED_IN) &&
-          this.isTabsSyncTurnedOn_());
-  }
 
   protected isTabsSyncTurnedOn_(): boolean {
     return this.historyIdentityState_.tabsSync === SyncState.TURNED_ON;
@@ -362,20 +301,6 @@ export class HistorySyncedDeviceManagerElement extends
         this.isTabsSyncTurnedOn_() && this.syncedDevices_.length === 0;
   }
 
-  /**
-   * Shows the signin guide when the user is not signed in, signin is allowed
-   * and not in a guest session.
-   */
-  protected showSignInGuide_(): boolean {
-    const show = this.isSignInState_(HistorySignInState.SIGNED_OUT) &&
-        !this.guestSession_ && this.signInAllowed_;
-    if (show) {
-      BrowserProxyImpl.getInstance().recordAction(
-          'Signin_Impression_FromRecentTabs');
-    }
-
-    return show;
-  }
 
   /**
    * Decide what message should be displayed when user is logged in and there
@@ -423,14 +348,8 @@ export class HistorySyncedDeviceManagerElement extends
     this.syncedDevices_ = devices;
   }
 
-  /**
-   * Get called when user's sign in state changes, this will affect UI of synced
-   * tabs page. Sign in promo gets displayed when user is signed out, and
-   * different messages are shown when there are no synced tabs.
-   */
+  /** Updates the device list and empty state when account identity changes. */
   private onIdentityStateChanged_(previous: HistoryIdentityState|null) {
-    this.maybeRecordSigninPendingOffered_();
-
     if (previous === null) {
       return;
     }
@@ -447,7 +366,7 @@ export class HistorySyncedDeviceManagerElement extends
         return;
       }
     } else if (this.isSignInState_(HistorySignInState.SIGNED_OUT)) {
-      // User signed out, clear synced device list and show the sign in promo.
+      // User signed out, clear synced device list.
       this.clearDisplayedSyncedDevices_();
       this.sessionList = null;
       return;
@@ -460,26 +379,6 @@ export class HistorySyncedDeviceManagerElement extends
     this.updateSyncedDevices_();
   }
 
-  private maybeRecordSigninPendingOffered_() {
-    if (!this.replaceSyncPromosWithSignInPromos_) {
-      return;
-    }
-
-    // Reset the flag if the state changes away from SIGNED_IN_PAUSED.
-    if (!this.isSignInState_(HistorySignInState.SIGN_IN_PENDING)) {
-      this.signinPausedImpressionRecorded_ = false;
-      return;
-    }
-
-    // Don't record twice.
-    if (this.signinPausedImpressionRecorded_) {
-      return;
-    }
-
-    BrowserProxyImpl.getInstance().recordSigninPendingOffered(
-        AccessPoint.kRecentTabs);
-    this.signinPausedImpressionRecorded_ = true;
-  }
 
   private searchTermChanged_() {
     this.clearDisplayedSyncedDevices_();
