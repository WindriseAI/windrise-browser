diff --git a/chrome/browser/about_flags.cc b/chrome/browser/about_flags.cc
index 05bbf0b..ecf48da 100644
--- a/chrome/browser/about_flags.cc
+++ b/chrome/browser/about_flags.cc
@@ -11429,6 +11429,17 @@ const FeatureEntry kFeatureEntries[] = {
      FEATURE_VALUE_TYPE(display::features::kFastDrmMasterDrop)},
 #endif  // BUILDFLAG(IS_CHROMEOS)
 
+#if !BUILDFLAG(IS_ANDROID)
+    {"enable-browseros-alpha-features",
+     flag_descriptions::kBrowserOsAlphaFeaturesName,
+     flag_descriptions::kBrowserOsAlphaFeaturesDescription, kOsDesktop,
+     FEATURE_VALUE_TYPE(features::kBrowserOsAlphaFeatures)},
+
+    {"enable-browseros-keyboard-shortcuts",
+     flag_descriptions::kBrowserOsKeyboardShortcutsName,
+     flag_descriptions::kBrowserOsKeyboardShortcutsDescription, kOsDesktop,
+     FEATURE_VALUE_TYPE(features::kBrowserOsKeyboardShortcuts)},
+#endif
 #if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_LINUX)
     {"automatic-usb-detach", flag_descriptions::kAutomaticUsbDetachName,
      flag_descriptions::kAutomaticUsbDetachDescription, kOsAndroid | kOsLinux,
