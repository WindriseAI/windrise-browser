diff --git a/chrome/common/pref_names.h b/chrome/common/pref_names.h
index 980cb73..181306b 100644
--- a/chrome/common/pref_names.h
+++ b/chrome/common/pref_names.h
@@ -916,6 +916,8 @@ inline constexpr char kImportDialogSavedPasswords[] =
     "import_dialog_saved_passwords";
 inline constexpr char kImportDialogSearchEngine[] =
     "import_dialog_search_engine";
+inline constexpr char kImportDialogExtensions[] = "import_dialog_extensions";
+inline constexpr char kImportDialogCookies[] = "import_dialog_cookies";
 
 // A dictionary mapping foreign session tags (see
 // `sync_sessions::SyncedSession::GetSessionTag()`) to `true` for synced devices
@@ -3294,6 +3296,9 @@ inline constexpr char kCpuPerformanceTierOverride[] =
 // Value indicating that the CPU performance tier has not been overridden.
 inline constexpr int kCpuPerformanceTierOverrideNone = -1;
 
+// NOTE: Other BrowserOS prefs have been moved to
+// chrome/browser/browseros/core/browseros_prefs.h
+
 }  // namespace prefs
 
 #endif  // CHROME_COMMON_PREF_NAMES_H_
