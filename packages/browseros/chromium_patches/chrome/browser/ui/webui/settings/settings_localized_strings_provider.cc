diff --git a/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc b/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
index 0bdda85eaa..182fe6291a 100644
--- a/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
+++ b/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
@@ -14,6 +14,7 @@
 #include "base/json/json_writer.h"
 #include "base/strings/escape.h"
 #include "base/strings/string_number_conversions.h"
+#include "base/strings/string_util.h"
 #include "base/strings/utf_string_conversions.h"
 #include "build/branding_buildflags.h"
 #include "build/build_config.h"
@@ -343,6 +344,7 @@ void AddAboutStrings(content::WebUIDataSource* html_source, Profile* profile) {
       {"aboutGetHelpUsingChrome", IDS_SETTINGS_GET_HELP_USING_CHROME},
       {"aboutPageTitle", IDS_SETTINGS_ABOUT_PROGRAM},
       {"aboutProductTitle", IDS_PRODUCT_NAME},
+      {"aboutFontLicense", IDS_SETTINGS_ABOUT_PAGE_FONT},
       {"aboutLearnMoreUpdatingErrors",
        IDS_SETTINGS_ABOUT_PAGE_LEARN_MORE_UPDATE_ERRORS},
       {"aboutLearnMoreSystemRequirements",
@@ -359,6 +361,43 @@ void AddAboutStrings(content::WebUIDataSource* html_source, Profile* profile) {
 
   html_source->AddString("managementPage",
                          GetDeviceManagedUiHelpLabel(profile));
+  // 关于页「源代码（开源）」一行：地址不进译文，在这里填，安装器 SOURCE.txt 同址。
+  html_source->AddString(
+      "aboutSourceCode",
+      l10n_util::GetStringFUTF16(IDS_SETTINGS_ABOUT_PAGE_SOURCE_CODE,
+                                 u"github.com/WindriseAI/windrise-browser"));
+  // 关于页合规信息：隐私政策 / 用户协议按界面语言选站（判法和菜单帮助一致，
+  // chrome_pages.cc）。运营主体与备案号照官网页脚原文，软件里不另编主体信息。
+  const std::string& windrise_locale = g_browser_process->GetApplicationLocale();
+  const bool windrise_is_chinese =
+      base::EqualsCaseInsensitiveASCII(windrise_locale, "zh") ||
+      base::StartsWith(windrise_locale, "zh-",
+                       base::CompareCase::INSENSITIVE_ASCII);
+  html_source->AddString(
+      "aboutPrivacyUrl", base::ASCIIToUTF16(windrise_is_chinese
+                                                ? kWindrisePrivacyChineseURL
+                                                : kWindrisePrivacyURL));
+  html_source->AddString(
+      "aboutTermsUrl", base::ASCIIToUTF16(windrise_is_chinese
+                                              ? kWindriseTermsChineseURL
+                                              : kWindriseTermsURL));
+  const std::u16string windrise_operator =
+      u"邯郸市丛台区风起网络科技工作室（个体工商户）";
+  const std::u16string windrise_icp = u"冀ICP备2026049007号-1";
+  html_source->AddString("aboutOperator", windrise_operator);
+  html_source->AddString("aboutIcp", windrise_icp);
+  // 关于页显示用：两条链接行的字；备案号一行小字（只在中文界面显示，放在整页最下面；
+  // 英文官网页脚同样不放，英文界面也不出现汉字）。运营主体只留数据键，界面不显示。
+  html_source->AddLocalizedString("aboutPrivacyLabel",
+                                  IDS_SETTINGS_ABOUT_PAGE_WINDRISE_PRIVACY);
+  html_source->AddLocalizedString("aboutTermsLabel",
+                                  IDS_SETTINGS_ABOUT_PAGE_WINDRISE_TERMS);
+  html_source->AddString(
+      "aboutIcpLine",
+      l10n_util::GetStringFUTF16(IDS_SETTINGS_ABOUT_PAGE_WINDRISE_ICP,
+                                 windrise_icp));
+  html_source->AddBoolean("aboutShowIcp", windrise_is_chinese);
+
   html_source->AddString(
       "aboutUpgradeUpToDate",
 #if BUILDFLAG(IS_CHROMEOS)
@@ -1278,6 +1317,8 @@ void AddImportDataStrings(content::WebUIDataSource* html_source) {
       {"importCommit", IDS_SETTINGS_IMPORT_COMMIT},
       {"noProfileFound", IDS_SETTINGS_IMPORT_NO_PROFILE_FOUND},
       {"importSuccess", IDS_SETTINGS_IMPORT_SUCCESS},
+      {"importDialogExtensions", IDS_SETTINGS_IMPORT_EXTENSIONS_CHECKBOX},
+      {"importDialogCookies", IDS_SETTINGS_IMPORT_COOKIES_CHECKBOX},
   };
   html_source->AddLocalizedStrings(kLocalizedStrings);
 }
