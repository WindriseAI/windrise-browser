diff --git a/chrome/browser/resources/settings/about_page/about_page.html.ts b/chrome/browser/resources/settings/about_page/about_page.html.ts
index b9ac4e634feddf9e14a6da7c36fcb49fa143c032..020114b4a45e41fbbf35ead5fbd252e61683582b 100644
--- a/chrome/browser/resources/settings/about_page/about_page.html.ts
+++ b/chrome/browser/resources/settings/about_page/about_page.html.ts
@@ -13,7 +13,7 @@ export function getHtml(this: SettingsAboutPageElement) {
   return html`<!--_html_template_start_-->
 <settings-section page-title="$i18n{aboutPageTitle}">
   <div class="cr-row two-line first">
-    <img id="productLogo" @click="${this.onProductLogoClick_}"
+    <img id="productLogo"
         srcset="chrome://theme/current-channel-logo@1x 1x,
                 chrome://theme/current-channel-logo@2x 2x"
         alt="$i18n{aboutProductLogoAlt}"
@@ -40,11 +40,6 @@ export function getHtml(this: SettingsAboutPageElement) {
         <div role="alert" aria-live="polite"
             .innerHTML="${this.getUpdateStatusMessage_()}">
         </div>
-        <a ?hidden="${!this.shouldShowLearnMoreLink_()}" target="_blank"
-            href="https://support.google.com/chrome?p=update_error"
-            aria-label="$i18nPolymer{aboutLearnMoreUpdatingErrors}">
-          $i18n{learnMore}
-        </a>
       </div>
       <span id="deprecationWarning"
           ?hidden="${!this.obsoleteSystemInfo_.obsolete}">
@@ -75,12 +70,6 @@ export function getHtml(this: SettingsAboutPageElement) {
         @click="${this.onPromoteUpdaterClick_}">
       <div class="flex">
         ${this.promoteUpdaterStatus_.text}
-        <a href="https://support.google.com/chrome/answer/95414"
-            target="_blank" id="updaterLearnMore"
-            @click="${this.onLearnMoreClick_}"
-            aria-label="$i18nPolymer{aboutLearnMoreUpdating}">
-          $i18n{learnMore}
-        </a>
       </div>
       <cr-icon-button class="subpage-arrow"
           ?hidden="${!this.promoteUpdaterStatus_.actionable}"
@@ -91,6 +80,8 @@ export function getHtml(this: SettingsAboutPageElement) {
 </if>
   <cr-link-row class="hr" id="help" @click="${this.onHelpClick_}"
       label="$i18n{aboutGetHelpUsingChrome}" external></cr-link-row>
+  <cr-link-row class="hr" id="sourceCode" @click="${this.onSourceCodeClick_}"
+      label="$i18n{aboutSourceCode}" external></cr-link-row>
 <if expr="_google_chrome">
   <cr-link-row class="hr" id="reportIssue" @click="${this.onReportIssueClick_}"
       ?hidden="${!this.feedbackAllowedPref_?.value}"
@@ -108,10 +99,19 @@ export function getHtml(this: SettingsAboutPageElement) {
 <settings-section>
   <div class="info-sections">
     <div class="info-section">
-      <div class="secondary">$i18n{aboutProductTitle}</div>
       <div class="secondary">$i18n{aboutProductCopyright}</div>
     </div>
 
+    <div class="info-section">
+      <a class="secondary" id="fontLicense" href="#" role="button"
+          aria-expanded="${this.showFontLicense_}"
+          @click="${this.onFontLicenseClick_}">$i18n{aboutFontLicense}</a>
+      ${this.fontLicenseText_ ? html`
+        <pre id="fontLicenseText" ?hidden="${!this.showFontLicense_}"
+            >${this.fontLicenseText_}</pre>
+      ` : ''}
+    </div>
+
     <div class="info-section">
       <div class="secondary">$i18nRaw{aboutProductLicense}</div>
     </div>
