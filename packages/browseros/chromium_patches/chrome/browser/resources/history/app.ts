diff --git a/chrome/browser/resources/history/app.ts b/chrome/browser/resources/history/app.ts
index 5f344c418c..733f6dc241 100644
--- a/chrome/browser/resources/history/app.ts
+++ b/chrome/browser/resources/history/app.ts
@@ -10,10 +10,6 @@ import 'chrome://resources/cr_elements/cr_lazy_render/cr_lazy_render_lit.js';
 import 'chrome://resources/cr_elements/cr_page_selector/cr_page_selector.js';
 import 'chrome://resources/cr_elements/cr_tabs/cr_tabs.js';
 import './history_embeddings_promo.js';
-// <if expr="not is_chromeos">
-import './history_cross_device_signin_promo.js';
-import './history_sync_promo.js';
-// </if>
 import './history_list.js';
 import './history_toolbar.js';
 import './history_filter_chips.js';
@@ -135,11 +131,6 @@ export class HistoryAppElement extends HistoryAppElementBase {
         type: Boolean,
         reflect: true,
       },
-      // <if expr="not is_chromeos">
-      unoPhase2FollowUpEnabled_: {type: Boolean},
-      shouldShowHistorySyncPromo_: {type: Boolean},
-      shouldShowHistoryCrossDeviceSigninPromo_: {type: Boolean},
-      // </if>
       contentPage_: {type: String},
       tabsContentPage_: {type: String},
       // The id of the currently selected page.
@@ -194,12 +185,6 @@ export class HistoryAppElement extends HistoryAppElementBase {
   };
   protected accessor enableHistoryEmbeddings_: boolean =
       loadTimeData.getBoolean('enableHistoryEmbeddings');
-  // <if expr="not is_chromeos">
-  protected accessor unoPhase2FollowUpEnabled_: boolean =
-      loadTimeData.getBoolean('unoPhase2FollowUp');
-  protected accessor shouldShowHistorySyncPromo_: boolean = false;
-  protected accessor shouldShowHistoryCrossDeviceSigninPromo_: boolean = false;
-  // </if>
   protected accessor hasDrawer_: boolean = false;
   protected accessor historyClustersEnabled_: boolean =
       loadTimeData.getBoolean('isHistoryClustersEnabled');
@@ -342,14 +327,6 @@ export class HistoryAppElement extends HistoryAppElementBase {
         this.callbackRouter_.onHasOtherFormsChanged.addListener(
             (hasOtherForms: boolean) =>
                 this.onHasOtherFormsChanged_(hasOtherForms));
-    // <if expr="not is_chromeos">
-    BrowserProxyImpl.getInstance()
-        .handler.shouldShowHistoryPageHistorySyncPromo()
-        .then(
-            ({shouldShow}) =>
-                this.handleShouldShowHistoryPageHistorySyncPromoChanged_(
-                    shouldShow));
-    // </if>
   }
 
   override disconnectedCallback() {
@@ -854,30 +831,6 @@ export class HistoryAppElement extends HistoryAppElementBase {
     this.hasDrawer_ = enabled;
   }
 
-  // <if expr="not is_chromeos">
-  // History sync promo is shown based on the following conditions:
-  // 1. UNO phase 2 follow up feature flag is enabled.
-  // 2. Should be shown based on user prefs (the promo was closed < 5 times)
-  // 3. History sync is not disabled.
-  // 4. User is not already signed in and syncing history.
-  protected shouldShowHistoryPageHistorySyncPromo_(): boolean {
-    return this.unoPhase2FollowUpEnabled_ && this.shouldShowHistorySyncPromo_ &&
-        this.identityState_.historySync !== SyncState.DISABLED &&
-        !(this.identityState_.signIn === HistorySignInState.SIGNED_IN &&
-          this.identityState_.historySync === SyncState.TURNED_ON);
-  }
-
-  private handleShouldShowHistoryPageHistorySyncPromoChanged_(
-      shouldShowHistorySyncPromo: boolean) {
-    this.shouldShowHistorySyncPromo_ = shouldShowHistorySyncPromo;
-  }
-
-  protected onShouldShowHistoryCrossDeviceSigninPromo_(
-      e: CustomEvent<{shouldShow: boolean}>) {
-    this.shouldShowHistoryCrossDeviceSigninPromo_ = e.detail.shouldShow;
-  }
-  // </if>
-
   protected shouldShowHistoryEmbeddings_(): boolean {
     if (!loadTimeData.getBoolean('enableHistoryEmbeddings')) {
       return false;
