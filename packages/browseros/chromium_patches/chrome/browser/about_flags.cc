diff --git a/chrome/browser/about_flags.cc b/chrome/browser/about_flags.cc
index 0f43cb2b3c..c8f9844b79 100644
--- a/chrome/browser/about_flags.cc
+++ b/chrome/browser/about_flags.cc
@@ -11488,6 +11488,17 @@ const FeatureEntry kFeatureEntries[] = {
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
