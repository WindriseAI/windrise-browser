diff --git a/chrome/browser/importer/importer_list.cc b/chrome/browser/importer/importer_list.cc
index 62546b572bab8..df99dae8f9719 100644
--- a/chrome/browser/importer/importer_list.cc
+++ b/chrome/browser/importer/importer_list.cc
@@ -6,10 +6,16 @@

 #include <stdint.h>

+#include "base/files/file.h"
+#include "base/files/file_util.h"
 #include "base/functional/bind.h"
+#include "base/json/json_reader.h"
+#include "base/path_service.h"
+#include "base/strings/utf_string_conversions.h"
 #include "base/task/task_traits.h"
 #include "base/task/thread_pool.h"
 #include "base/threading/scoped_blocking_call.h"
+#include "base/values.h"
 #include "build/build_config.h"
 #include "chrome/browser/shell_integration.h"
 #include "chrome/common/importer/firefox_importer_utils.h"
@@ -17,6 +23,7 @@
 #include "chrome/grit/generated_resources.h"
 #include "components/user_data_importer/common/importer_data_types.h"
 #include "ui/base/l10n/l10n_util.h"
+#include "base/logging.h"

 #if BUILDFLAG(IS_MAC)
 #include "base/apple/foundation_util.h"
@@ -29,6 +36,279 @@
 
  namespace {
 
+// Forward declarations for platform-specific Chromium-family user data folders
+base::FilePath GetChromeUserDataFolder();
+base::FilePath GetChromiumEdgeUserDataFolder();
+
+bool FilePresentOrLocked(const base::FilePath& path) {
+  if (path.empty()) {
+    return false;
+  }
+  if (base::PathExists(path)) {
+    return true;
+  }
+  base::File probe(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
+  return probe.IsValid() ||
+         probe.error_details() == base::File::FILE_ERROR_IN_USE;
+}
+
+constexpr char kChromeNoHostedDomainFound[] = "NO_HOSTED_DOMAIN";
+
+std::string GetChromeAccountName(const base::DictValue& profile) {
+  const std::string* user_name = profile.FindString("user_name");
+  if (user_name && !user_name->empty()) {
+    return *user_name;
+  }
+
+  const std::string* gaia_name = profile.FindString("gaia_name");
+  return gaia_name ? *gaia_name : std::string();
+}
+
+bool IsManagedChromeProfile(const base::DictValue& profile) {
+  const std::string* hosted_domain = profile.FindString("hosted_domain");
+  if (hosted_domain && !hosted_domain->empty() &&
+      *hosted_domain != kChromeNoHostedDomainFound) {
+    return true;
+  }
+
+  const std::string* managed_user_id = profile.FindString("managed_user_id");
+  return managed_user_id && !managed_user_id->empty();
+}
+
+// Chrome importer helper functions (cross-platform)
+bool HasExtensionsToImport(const base::FilePath& preferences_path) {
+  LOG(INFO) << "windrise: Checking for extensions in: " << preferences_path.AsUTF8Unsafe();
+
+  std::string preferences_content;
+  if (!base::ReadFileToString(preferences_path, &preferences_content)) {
+    LOG(INFO) << "windrise: Failed to read preferences file: " << preferences_path.AsUTF8Unsafe();
+    return false;
+  }
+
+  std::optional<base::DictValue> preferences =
+      base::JSONReader::ReadDict(preferences_content, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
+  if (!preferences) {
+    LOG(INFO) << "windrise: Failed to parse preferences file as JSON: " << preferences_path.AsUTF8Unsafe();
+    return false;
+  }
+
+  // Extensions are stored in extensions.settings in Chrome preferences
+  const base::DictValue* extensions_dict =
+      preferences->FindDictByDottedPath("extensions.settings");
+  if (!extensions_dict) {
+    LOG(INFO) << "windrise: No extensions.settings found in preferences file";
+    return false;
+  }
+
+  LOG(INFO) << "windrise: Found extensions.settings with " << extensions_dict->size() << " entries";
+
+  // Check for at least one valid extension
+  int examined_extensions = 0;
+  for (const auto [key, value] : *extensions_dict) {
+    examined_extensions++;
+    if (!value.is_dict()) {
+      continue;
+    }
+
+    const base::DictValue& dict = value.GetDict();
+
+    // Only count if:
+    // 1. It's from the Chrome Web Store
+    // 2. It's not installed by default
+    // 3. It's enabled (we're being lenient here, importing disabled ones too for now)
+
+    if (dict.FindBool("was_installed_by_default").value_or(true)) {
+      LOG(INFO) << "windrise: Extension " << key << " was installed by default, skipping";
+      continue;  // Skip default extensions
+    }
+
+    if (!dict.FindBool("from_webstore").value_or(false)) {
+      LOG(INFO) << "windrise: Extension " << key << " is not from the web store, skipping";
+      continue;  // Skip non-webstore extensions
+    }
+    return true;
+  }
+
+  LOG(INFO) << "windrise: Examined " << examined_extensions << " extensions, none qualified for import";
+  return false;
+}
+
+bool ChromeImporterCanImport(const base::FilePath& profile_path, uint16_t* services) {
+  DCHECK(services);
+  *services = user_data_importer::NONE;
+
+  if (!FilePresentOrLocked(profile_path))
+    return false;
+
+  base::FilePath bookmarks_path = profile_path.Append(FILE_PATH_LITERAL("Bookmarks"));
+  base::FilePath history_path = profile_path.Append(FILE_PATH_LITERAL("History"));
+  base::FilePath passwords_path = profile_path.Append(FILE_PATH_LITERAL("Login Data"));
+  base::FilePath network_cookies_path =
+      profile_path.Append(FILE_PATH_LITERAL("Network"))
+          .Append(FILE_PATH_LITERAL("Cookies"));
+  base::FilePath cookies_path = profile_path.Append(FILE_PATH_LITERAL("Cookies"));
+  base::FilePath preferences_path = profile_path.Append(FILE_PATH_LITERAL("Preferences"));
+  base::FilePath secure_preferences_path = profile_path.Append(FILE_PATH_LITERAL("Secure Preferences"));
+
+  if (FilePresentOrLocked(bookmarks_path))
+    *services |= user_data_importer::FAVORITES;
+
+  if (FilePresentOrLocked(history_path))
+    *services |= user_data_importer::HISTORY;
+
+  if (FilePresentOrLocked(passwords_path))
+    *services |= user_data_importer::PASSWORDS;
+
+  if (FilePresentOrLocked(network_cookies_path) || FilePresentOrLocked(cookies_path))
+    *services |= user_data_importer::COOKIES;
+
+  if (FilePresentOrLocked(preferences_path)) {
+    *services |= user_data_importer::AUTOFILL_FORM_DATA;
+    *services |= user_data_importer::SEARCH_ENGINES;
+
+    // Check for extensions in preferences
+    if (HasExtensionsToImport(preferences_path) ||
+        (FilePresentOrLocked(secure_preferences_path) &&
+         HasExtensionsToImport(secure_preferences_path))) {
+      *services |= user_data_importer::EXTENSIONS;
+    }
+  }
+
+  return *services != user_data_importer::NONE;
+}
+
+base::ListValue GetChromeSourceProfiles(const base::FilePath& local_state_path) {
+  base::ListValue profiles;
+
+  if (FilePresentOrLocked(local_state_path)) {
+    std::string local_state_content;
+    if (base::ReadFileToString(local_state_path, &local_state_content)) {
+      std::optional<base::DictValue> local_state_dict =
+          base::JSONReader::ReadDict(local_state_content,
+                                     base::JSON_PARSE_CHROMIUM_EXTENSIONS);
+
+      if (local_state_dict) {
+        const auto* profile_dict = local_state_dict->FindDict("profile");
+        if (profile_dict) {
+          const auto* info_cache = profile_dict->FindDict("info_cache");
+          if (info_cache) {
+            for (const auto value : *info_cache) {
+              const auto* profile = value.second.GetIfDict();
+              if (!profile)
+                continue;
+
+              auto* name = profile->FindString("name");
+              if (!name)
+                continue;
+
+              base::DictValue entry;
+              entry.Set("id", value.first);
+              entry.Set("name", *name);
+              entry.Set("account_name", GetChromeAccountName(*profile));
+              entry.Set("is_managed", IsManagedChromeProfile(*profile));
+              profiles.Append(std::move(entry));
+            }
+          }
+        }
+      }
+    }
+  }
+
+  // If no profiles were found, add the default one
+  if (profiles.empty()) {
+    base::DictValue entry;
+    entry.Set("id", "Default");
+    entry.Set("name", "Default");
+    entry.Set("account_name", "");
+    entry.Set("is_managed", false);
+    profiles.Append(std::move(entry));
+  }
+
+  return profiles;
+}
+
+bool DetectChromiumProfiles(
+    const base::FilePath& user_data_path,
+    const std::u16string& browser_name,
+    std::vector<user_data_importer::SourceProfile>* profiles) {
+  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
+                                               base::BlockingType::MAY_BLOCK);
+
+  if (user_data_path.empty() || !FilePresentOrLocked(user_data_path))
+    return false;
+
+  // Get the list of profiles from Local State
+  base::FilePath local_state_path =
+      user_data_path.Append(FILE_PATH_LITERAL("Local State"));
+  base::ListValue source_profiles = GetChromeSourceProfiles(local_state_path);
+
+  bool added_any = false;
+  // Add each profile
+  for (const auto& value : source_profiles) {
+    const auto* dict = value.GetIfDict();
+    if (!dict)
+      continue;
+
+    const std::string* profile_id = dict->FindString("id");
+    const std::string* name = dict->FindString("name");
+    const std::string* account_name = dict->FindString("account_name");
+
+    if (!profile_id || !name)
+      continue;
+
+    // Profile ids from Local State are UTF-8 ("Default", "Profile 1", ...);
+    // convert properly rather than widening bytes so the path is correct on
+    // Windows (FilePath uses wchar_t there).
+    base::FilePath profile_folder =
+        user_data_path.Append(base::FilePath::FromUTF8Unsafe(*profile_id).value());
+    uint16_t services = user_data_importer::NONE;
+
+    if (!ChromeImporterCanImport(profile_folder, &services))
+      continue;
+
+    user_data_importer::SourceProfile source;
+    source.importer_name = browser_name;
+    if (*profile_id != "Default") {
+      source.profile = base::UTF8ToUTF16(*name);
+    }
+    source.importer_type = user_data_importer::TYPE_CHROME;
+    source.services_supported = services;
+    source.source_path = profile_folder;
+    if (account_name) {
+      source.account_name = base::UTF8ToUTF16(*account_name);
+    }
+    source.is_managed = dict->FindBool("is_managed").value_or(false);
+    profiles->push_back(source);
+    added_any = true;
+  }
+  return added_any;
+}
+
+void DetectChromeProfiles(
+    std::vector<user_data_importer::SourceProfile>* profiles) {
+  DetectChromiumProfiles(GetChromeUserDataFolder(),
+                         l10n_util::GetStringUTF16(IDS_IMPORT_FROM_CHROME),
+                         profiles);
+}
+
+bool DetectChromiumEdgeProfiles(
+    std::vector<user_data_importer::SourceProfile>* profiles) {
+  const base::FilePath edge_data_dir = GetChromiumEdgeUserDataFolder();
+  const bool has_modern_edge_dir =
+      !edge_data_dir.empty() && FilePresentOrLocked(edge_data_dir);
+#if BUILDFLAG(IS_WIN)
+  const std::u16string edge_name =
+      l10n_util::GetStringUTF16(IDS_IMPORT_FROM_EDGE);
+#else
+  const std::u16string edge_name = u"Microsoft Edge";
+#endif
+  const bool added = DetectChromiumProfiles(edge_data_dir, edge_name, profiles);
+  // When the modern Chromium Edge User Data directory is present on disk,
+  // return true even if Local State was locked/unreadable so Windows never
+  // falls back to legacy UWP EdgeHTML favorites under the same Edge label.
+  return added || has_modern_edge_dir;
+}
+
 #if BUILDFLAG(IS_WIN)
 void DetectIEProfiles(
     std::vector<user_data_importer::SourceProfile>* profiles) {
@@ -51,27 +331,59 @@ void DetectEdgeProfiles(
   if (!importer::EdgeImporterCanImport()) {
     return;
   }
   user_data_importer::SourceProfile edge;
-  edge.importer_name = l10n_util::GetStringUTF16(IDS_IMPORT_FROM_EDGE);
+  edge.importer_name =
+      l10n_util::GetStringUTF16(IDS_IMPORT_FROM_EDGE) + u" (Legacy EdgeHTML)";
   edge.importer_type = user_data_importer::TYPE_EDGE;
   edge.services_supported = user_data_importer::FAVORITES;
   edge.source_path = importer::GetEdgeDataFilePath();
   profiles->push_back(edge);
 }

 void DetectBuiltinWindowsProfiles(
-    std::vector<user_data_importer::SourceProfile>* profiles) {
+    std::vector<user_data_importer::SourceProfile>* profiles,
+    bool include_legacy_edge) {
   if (shell_integration::IsIEDefaultBrowser()) {
     DetectIEProfiles(profiles);
-    DetectEdgeProfiles(profiles);
+    if (include_legacy_edge)
+      DetectEdgeProfiles(profiles);
   } else {
-    DetectEdgeProfiles(profiles);
+    if (include_legacy_edge)
+      DetectEdgeProfiles(profiles);
     DetectIEProfiles(profiles);
   }
 }

 #endif  // BUILDFLAG(IS_WIN)

+#if BUILDFLAG(IS_WIN)
+// Windows-specific Chrome user data folder getter
+base::FilePath GetChromeUserDataFolder() {
+  base::FilePath result;
+  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &result))
+    return base::FilePath();
+
+  result = result.Append(FILE_PATH_LITERAL("Google"));
+  result = result.Append(FILE_PATH_LITERAL("Chrome"));
+  result = result.Append(FILE_PATH_LITERAL("User Data"));
+
+  return result;
+}
+
+// Windows-specific modern Chromium Edge user data folder getter
+base::FilePath GetChromiumEdgeUserDataFolder() {
+  base::FilePath result;
+  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &result))
+    return base::FilePath();
+
+  result = result.Append(FILE_PATH_LITERAL("Microsoft"));
+  result = result.Append(FILE_PATH_LITERAL("Edge"));
+  result = result.Append(FILE_PATH_LITERAL("User Data"));
+
+  return result;
+}
+#endif  // BUILDFLAG(IS_WIN)
+
 #if BUILDFLAG(IS_MAC)
 void DetectSafariProfiles(
     std::vector<user_data_importer::SourceProfile>* profiles) {
@@ -88,8 +400,48 @@ void DetectSafariProfiles(
   safari.services_supported = items;
   profiles->push_back(safari);
 }
+
+// macOS-specific Chrome user data folder getter
+base::FilePath GetChromeUserDataFolder() {
+  base::FilePath result = base::apple::GetUserLibraryPath();
+  return result.Append("Application Support/Google/Chrome");
+}
+
+// macOS-specific modern Chromium Edge user data folder getter
+base::FilePath GetChromiumEdgeUserDataFolder() {
+  base::FilePath result = base::apple::GetUserLibraryPath();
+  return result.Append("Application Support/Microsoft Edge");
+}
+
+// These functions have been moved outside the platform blocks above
 #endif  // BUILDFLAG(IS_MAC)

+#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
+// Linux-specific Chrome user data folder getter
+base::FilePath GetChromeUserDataFolder() {
+  const char* home = getenv("HOME");
+  if (!home)
+    return base::FilePath();
+
+  base::FilePath result(home);
+  result = result.Append(".config");
+  result = result.Append("google-chrome");
+  return result;
+}
+
+// Linux-specific modern Chromium Edge user data folder getter
+base::FilePath GetChromiumEdgeUserDataFolder() {
+  const char* home = getenv("HOME");
+  if (!home)
+    return base::FilePath();
+
+  base::FilePath result(home);
+  result = result.Append(".config");
+  result = result.Append("microsoft-edge");
+  return result;
+}
+#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
+
 // |locale|: The application locale used for lookups in Firefox's
 // locale-specific search engines feature (see firefox_importer.cc for
 // details).
@@ -170,20 +522,37 @@ std::vector<user_data_importer::SourceProfile> DetectSourceProfilesWorker(
 #if BUILDFLAG(IS_WIN)
   if (shell_integration::IsFirefoxDefaultBrowser()) {
     DetectFirefoxProfiles(locale, &profiles);
-    DetectBuiltinWindowsProfiles(&profiles);
+    DetectChromeProfiles(&profiles);
+    const bool has_chromium_edge = DetectChromiumEdgeProfiles(&profiles);
+    const bool has_modern_edge_dir =
+        FilePresentOrLocked(GetChromiumEdgeUserDataFolder());
+    // Never misrepresent legacy EdgeHTML favorites as modern Chromium Edge
+    // when a modern Chromium Edge profile or user data dir is present.
+    DetectBuiltinWindowsProfiles(&profiles, !has_chromium_edge && !has_modern_edge_dir);
   } else {
-    DetectBuiltinWindowsProfiles(&profiles);
+    DetectChromeProfiles(&profiles);
+    const bool has_chromium_edge = DetectChromiumEdgeProfiles(&profiles);
+    const bool has_modern_edge_dir =
+        FilePresentOrLocked(GetChromiumEdgeUserDataFolder());
+    DetectBuiltinWindowsProfiles(&profiles, !has_chromium_edge && !has_modern_edge_dir);
     DetectFirefoxProfiles(locale, &profiles);
   }
 #elif BUILDFLAG(IS_MAC)
   if (shell_integration::IsFirefoxDefaultBrowser()) {
     DetectFirefoxProfiles(locale, &profiles);
     DetectSafariProfiles(&profiles);
+    DetectChromeProfiles(&profiles);
+    DetectChromiumEdgeProfiles(&profiles);
   } else {
     DetectSafariProfiles(&profiles);
+    DetectChromeProfiles(&profiles);
+    DetectChromiumEdgeProfiles(&profiles);
     DetectFirefoxProfiles(locale, &profiles);
   }
 #else
+  // Linux and other platforms
+  DetectChromeProfiles(&profiles);
+  DetectChromiumEdgeProfiles(&profiles);
   DetectFirefoxProfiles(locale, &profiles);
 #endif
   if (include_interactive_profiles) {
