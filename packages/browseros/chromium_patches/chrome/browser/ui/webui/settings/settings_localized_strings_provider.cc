diff --git a/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc b/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
index 0bdda85eaadb15c23754e4968950df2c4fa0e14f..748238677e40ac14b598ab6c66b0434d8ab5a6ae 100644
--- a/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
+++ b/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
@@ -343,6 +343,7 @@ void AddAboutStrings(content::WebUIDataSource* html_source, Profile* profile) {
       {"aboutGetHelpUsingChrome", IDS_SETTINGS_GET_HELP_USING_CHROME},
       {"aboutPageTitle", IDS_SETTINGS_ABOUT_PROGRAM},
       {"aboutProductTitle", IDS_PRODUCT_NAME},
+      {"aboutFontLicense", IDS_SETTINGS_ABOUT_PAGE_FONT},
       {"aboutLearnMoreUpdatingErrors",
        IDS_SETTINGS_ABOUT_PAGE_LEARN_MORE_UPDATE_ERRORS},
       {"aboutLearnMoreSystemRequirements",
@@ -359,6 +360,11 @@ void AddAboutStrings(content::WebUIDataSource* html_source, Profile* profile) {
 
   html_source->AddString("managementPage",
                          GetDeviceManagedUiHelpLabel(profile));
+  // 关于页「源代码（开源）」一行：地址不进译文，在这里填，安装器 SOURCE.txt 同址。
+  html_source->AddString(
+      "aboutSourceCode",
+      l10n_util::GetStringFUTF16(IDS_SETTINGS_ABOUT_PAGE_SOURCE_CODE,
+                                 u"github.com/WindriseAI/windrise-browser"));
   html_source->AddString(
       "aboutUpgradeUpToDate",
 #if BUILDFLAG(IS_CHROMEOS)
@@ -1278,6 +1284,8 @@ void AddImportDataStrings(content::WebUIDataSource* html_source) {
       {"importCommit", IDS_SETTINGS_IMPORT_COMMIT},
       {"noProfileFound", IDS_SETTINGS_IMPORT_NO_PROFILE_FOUND},
       {"importSuccess", IDS_SETTINGS_IMPORT_SUCCESS},
+      {"importDialogExtensions", IDS_SETTINGS_IMPORT_EXTENSIONS_CHECKBOX},
+      {"importDialogCookies", IDS_SETTINGS_IMPORT_COOKIES_CHECKBOX},
   };
   html_source->AddLocalizedStrings(kLocalizedStrings);
 }
