diff --git a/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc b/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
index 0bdda85eaa..052d5e8e46 100644
--- a/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
+++ b/chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc
@@ -1278,6 +1278,8 @@ void AddImportDataStrings(content::WebUIDataSource* html_source) {
       {"importCommit", IDS_SETTINGS_IMPORT_COMMIT},
       {"noProfileFound", IDS_SETTINGS_IMPORT_NO_PROFILE_FOUND},
       {"importSuccess", IDS_SETTINGS_IMPORT_SUCCESS},
+      {"importDialogExtensions", IDS_SETTINGS_IMPORT_EXTENSIONS_CHECKBOX},
+      {"importDialogCookies", IDS_SETTINGS_IMPORT_COOKIES_CHECKBOX},
   };
   html_source->AddLocalizedStrings(kLocalizedStrings);
 }
