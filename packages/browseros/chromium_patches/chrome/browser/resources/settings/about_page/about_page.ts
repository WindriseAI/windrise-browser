diff --git a/chrome/browser/resources/settings/about_page/about_page.ts b/chrome/browser/resources/settings/about_page/about_page.ts
index 30855eca88e6139226d36767e57cda53c44f72aa..6bbe1e8d5ae8eb6a72e3932117c0d38cb9bef26b 100644
--- a/chrome/browser/resources/settings/about_page/about_page.ts
+++ b/chrome/browser/resources/settings/about_page/about_page.ts
@@ -93,6 +93,11 @@ export class SettingsAboutPageElement extends SettingsAboutPageElementBase
       // </if>
 
       feedbackAllowedPref_: {type: Object},
+      fontLicenseText_: {type: String},
+      showFontLicense_: {type: Boolean},
+      agplLicenseText_: {type: String},
+      showAgplLicense_: {type: Boolean},
+      showIcp_: {type: Boolean},
     };
   }
 
@@ -127,6 +132,12 @@ export class SettingsAboutPageElement extends SettingsAboutPageElementBase
 
   protected accessor feedbackAllowedPref_:
       chrome.settingsPrivate.PrefObject<boolean>|undefined;
+  protected accessor fontLicenseText_: string = '';
+  protected accessor showFontLicense_: boolean = false;
+  protected accessor agplLicenseText_: string = '';
+  protected accessor showAgplLicense_: boolean = false;
+  // 备案号只在中文界面显示，放在整页最下面一行（判法在 settings_localized_strings_provider.cc）。
+  protected accessor showIcp_: boolean = loadTimeData.getBoolean('aboutShowIcp');
 
   private aboutBrowserProxy_: AboutPageBrowserProxy =
       AboutPageBrowserProxyImpl.getInstance();
@@ -224,6 +235,45 @@ export class SettingsAboutPageElement extends SettingsAboutPageElementBase
     this.aboutBrowserProxy_.openHelpPage();
   }
 
+  // 浏览器开源（AGPL）：源代码仓库。和安装器 SOURCE.txt 写的是同一个地址。
+  protected onSourceCodeClick_() {
+    window.open('https://github.com/WindriseAI/windrise-browser');
+  }
+
+  // 隐私政策、用户协议：地址由主进程按界面语言给（chrome/common/url_constants.h 唯一出处），这里不写地址。
+  protected onWindrisePrivacyClick_() {
+    window.open(loadTimeData.getString('aboutPrivacyUrl'));
+  }
+
+  protected onWindriseTermsClick_() {
+    window.open(loadTimeData.getString('aboutTermsUrl'));
+  }
+
+  // 字体声明平时只有一行小字，点一下在这一行下面展开协议全文，再点收起。
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
+  }
+
+  // AGPL-3.0 第 5(d) 条合规：无担保说明与页内展开许可证全文。
+  protected async onAgplLicenseClick_(e: Event) {
+    e.preventDefault();
+    if (!this.agplLicenseText_) {
+      const response =
+          await fetch('chrome://resources/windrise_license/LICENSE.txt');
+      const text = await response.text();
+      this.agplLicenseText_ = text.replace(/\0/g, '').trim();
+    }
+    this.showAgplLicense_ = !this.showAgplLicense_;
+  }
+
   protected onRelaunchClick_() {
     this.performRestart(RestartType.RELAUNCH);
   }
