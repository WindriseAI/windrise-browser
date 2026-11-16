diff --git a/chrome/browser/resources/settings/about_page/about_page.ts b/chrome/browser/resources/settings/about_page/about_page.ts
index 30855eca88e6139226d36767e57cda53c44f72aa..b79c01a6b11c7f066c425a937e853beaebd93a84 100644
--- a/chrome/browser/resources/settings/about_page/about_page.ts
+++ b/chrome/browser/resources/settings/about_page/about_page.ts
@@ -93,6 +93,8 @@ export class SettingsAboutPageElement extends SettingsAboutPageElementBase
       // </if>
 
       feedbackAllowedPref_: {type: Object},
+      fontLicenseText_: {type: String},
+      showFontLicense_: {type: Boolean},
     };
   }
 
@@ -127,6 +129,8 @@ export class SettingsAboutPageElement extends SettingsAboutPageElementBase
 
   protected accessor feedbackAllowedPref_:
       chrome.settingsPrivate.PrefObject<boolean>|undefined;
+  protected accessor fontLicenseText_: string = '';
+  protected accessor showFontLicense_: boolean = false;
 
   private aboutBrowserProxy_: AboutPageBrowserProxy =
       AboutPageBrowserProxyImpl.getInstance();
@@ -221,7 +225,26 @@ export class SettingsAboutPageElement extends SettingsAboutPageElementBase
   }
 
   protected onHelpClick_() {
-    this.aboutBrowserProxy_.openHelpPage();
+    window.open('http://docs.browseros.com/');
+  }
+
+  // 浏览器开源（AGPL）：源代码仓库。和安装器 SOURCE.txt 写的是同一个地址。
+  protected onSourceCodeClick_() {
+    window.open('https://github.com/WindriseAI/windrise-browser');
+  }
+
+  // 字体声明（kb/browser-ui-inventory-1106「已定」第 3 条；用户 11-06：「字体协议那个不用给它
+  // 那么多画面，写一行小小字儿就行了」）：平时只有一行小字，点一下在这一行下面展开协议全文，再点收起。
+  // 协议文件和字体一起原样打包在 chrome://resources/harmonyos_sans/，末尾补齐用的空字符显示时去掉。
+  protected async onFontLicenseClick_(e: Event) {
+    e.preventDefault();
+    if (!this.fontLicenseText_) {
+      const response =
+          await fetch('chrome://resources/harmonyos_sans/LICENSE.txt');
+      const text = await response.text();
+      this.fontLicenseText_ = text.replace(/\0/g, '').trim();
+    }
+    this.showFontLicense_ = !this.showFontLicense_;
   }
 
   protected onRelaunchClick_() {
