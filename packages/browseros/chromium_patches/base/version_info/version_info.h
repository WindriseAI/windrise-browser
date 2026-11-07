diff --git a/base/version_info/version_info.h b/base/version_info/version_info.h
index 3d83451..d7c3a9c 100644
--- a/base/version_info/version_info.h
+++ b/base/version_info/version_info.h
@@ -34,6 +34,11 @@ constexpr std::string_view GetVersionNumber() {
 }
 #endif
 
+// Returns the BrowserOS version number, e.g. "0.30.0.0".
+constexpr std::string_view GetBrowserOSVersionNumber() {
+  return BROWSEROS_VERSION;
+}
+
 // Returns the major component (aka the milestone) of the version as an int,
 // e.g. 6 when the version is "6.0.490.1".
 int GetMajorVersionNumberAsInt();
