diff --git a/components/search_engines/template_url_prepopulate_data_resolver.cc b/components/search_engines/template_url_prepopulate_data_resolver.cc
--- a/components/search_engines/template_url_prepopulate_data_resolver.cc
+++ b/components/search_engines/template_url_prepopulate_data_resolver.cc
@@ -10,17 +10,22 @@
 #include "base/debug/crash_logging.h"
 #include "base/debug/dump_without_crashing.h"
+#include "base/i18n/rtl.h"
 #include "base/logging.h"
 #include "base/metrics/histogram_functions.h"
+#include "base/strings/string_util.h"
+#include "components/country_codes/country_codes.h"
 #include "components/prefs/pref_service.h"
 #include "components/regional_capabilities/regional_capabilities_country_id.h"
 #include "components/regional_capabilities/regional_capabilities_service.h"
 #include "components/regional_capabilities/regional_capabilities_switches.h"
 #include "components/regional_capabilities/regional_capabilities_utils.h"
 #include "components/search_engines/keyword_table.h"
 #include "components/search_engines/keyword_web_data_service.h"
 #include "components/search_engines/template_url_data.h"
+#include "components/search_engines/template_url_data_util.h"
 #include "components/search_engines/template_url_prepopulate_data.h"
+#include "third_party/search_engines_data/resources/definitions/prepopulated_engines.h"
 #include "url/gurl.h"
 
 namespace TemplateURLPrepopulateData {
 
@@ -69,5 +74,34 @@
+namespace {
+
+bool IsSimplifiedChineseLocale(std::string_view locale) {
+  if (locale.empty()) {
+    return false;
+  }
+  if (base::EqualsCaseInsensitiveASCII(locale, "zh") ||
+      base::EqualsCaseInsensitiveASCII(locale, "zh-cn") ||
+      base::EqualsCaseInsensitiveASCII(locale, "zh_cn") ||
+      base::EqualsCaseInsensitiveASCII(locale, "zh-hans") ||
+      base::EqualsCaseInsensitiveASCII(locale, "zh_hans")) {
+    return true;
+  }
+  return base::StartsWith(locale, "zh-hans-",
+                          base::CompareCase::INSENSITIVE_ASCII) ||
+         base::StartsWith(locale, "zh_hans_",
+                          base::CompareCase::INSENSITIVE_ASCII);
+}
+
+}  // namespace
+
 std::unique_ptr<TemplateURLData> Resolver::GetFallbackSearch() const {
-  return TemplateURLPrepopulateData::GetPrepopulatedFallbackSearch(
-      profile_prefs_.get(),
-      regional_capabilities_->GetRegionalPrepopulatedEngines());
+  // Windrise defaults to Baidu for mainland China (CN) or Simplified Chinese
+  // UI locales so users without a VPN can search immediately; other regions
+  // keep Google. User, extension and policy choices retain their existing
+  // precedence in DefaultSearchManager.
+  if (regional_capabilities_->GetCountryId() ==
+          regional_capabilities::CountryIdHolder(
+              country_codes::CountryId("CN")) ||
+      IsSimplifiedChineseLocale(base::i18n::GetConfiguredLocale())) {
+    return TemplateURLDataFromPrepopulatedEngine(baidu);
+  }
+  return TemplateURLDataFromPrepopulatedEngine(google);
 }
