diff --git a/chrome/browser/resources/settings/people_page/people_page.html.ts b/chrome/browser/resources/settings/people_page/people_page.html.ts
index b330308..112c3d7 100644
--- a/chrome/browser/resources/settings/people_page/people_page.html.ts
+++ b/chrome/browser/resources/settings/people_page/people_page.html.ts
@@ -90,19 +90,23 @@ export function getHtml(this: SettingsPeoplePageElement) {
           </div>
         </cr-link-row>
 
+        <!-- Windrise: this build has no Google API key, so the sync and
+             Google services entries are empty shells. Keep the elements in
+             the DOM (getFocusConfig() and getAssociatedControlFor() look them
+             up by id) but never show them. -->
         <cr-link-row id="sync-setup"
             label="$i18n{syncAndNonPersonalizedServices}"
             .subLabel="${this.getSyncAndNonPersonalizedServicesSubtext_()}"
             @click="${this.onSyncClick_}"
             role-description="$i18n{subpageArrowRoleDescription}"
-            ?hidden="${this.shouldHideSyncSetupLinkRow_()}">
+            hidden>
         </cr-link-row>
 
         <cr-link-row id="google-services"
             label="$i18n{googleServicesPageTitle}"
             @click="${this.onGoogleServicesClick_}"
             role-description="$i18n{subpageArrowRoleDescription}"
-            ?hidden="${!this.shouldHideSyncSetupLinkRow_()}">
+            hidden>
         </cr-link-row>
 
 <if expr="not is_chromeos">
