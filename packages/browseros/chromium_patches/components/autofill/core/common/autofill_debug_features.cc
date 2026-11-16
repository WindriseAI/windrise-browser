diff --git a/components/autofill/core/common/autofill_debug_features.cc b/components/autofill/core/common/autofill_debug_features.cc
index 6acf5c60ef..6e1d14d370 100644
--- a/components/autofill/core/common/autofill_debug_features.cc
+++ b/components/autofill/core/common/autofill_debug_features.cc
@@ -140,7 +140,8 @@ BASE_FEATURE_PARAM(std::string,
 // autofill server API url up to the parent "directory" of the "query" and
 // "upload" resources.
 // i.e., https://other.autofill.server:port/tbproxy/af/
-BASE_FEATURE(kAutofillServerCommunication, base::FEATURE_ENABLED_BY_DEFAULT);
+// Windrise keeps local filling; both crowdsourcing query and upload use this gate.
+BASE_FEATURE(kAutofillServerCommunication, base::FEATURE_DISABLED_BY_DEFAULT);
 
 // When set to a non-zero value, the value is attached as an experiment ID to
 // all predictions requests to the Autofill server, as if it were an active GWS
