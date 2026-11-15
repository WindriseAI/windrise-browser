diff --git a/components/search_engines/template_url_prepopulate_data_resolver.cc b/components/search_engines/template_url_prepopulate_data_resolver.cc
--- a/components/search_engines/template_url_prepopulate_data_resolver.cc
+++ b/components/search_engines/template_url_prepopulate_data_resolver.cc
@@ -19,8 +19,10 @@
 #include "components/search_engines/keyword_table.h"
 #include "components/search_engines/keyword_web_data_service.h"
 #include "components/search_engines/template_url_data.h"
+#include "components/search_engines/template_url_data_util.h"
 #include "components/search_engines/template_url_prepopulate_data.h"
+#include "third_party/search_engines_data/resources/definitions/prepopulated_engines.h"
 #include "url/gurl.h"
 
 namespace TemplateURLPrepopulateData {
 
@@ -69,5 +71,5 @@
 std::unique_ptr<TemplateURLData> Resolver::GetFallbackSearch() const {
-  return TemplateURLPrepopulateData::GetPrepopulatedFallbackSearch(
-      profile_prefs_.get(),
-      regional_capabilities_->GetRegionalPrepopulatedEngines());
+  // Windrise's first default is Google; user, extension and policy choices
+  // retain their existing precedence in DefaultSearchManager.
+  return TemplateURLDataFromPrepopulatedEngine(google);
 }
