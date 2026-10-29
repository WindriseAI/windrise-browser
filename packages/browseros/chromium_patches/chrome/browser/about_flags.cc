diff --git a/chrome/browser/about_flags.cc b/chrome/browser/about_flags.cc
index ba175a0e376a962cf6bd09c8ae1b82819788ee9d..6b2719bcaf6efc3c57b19c7cc0c8fe3588ba0bce 100644
--- a/chrome/browser/about_flags.cc
+++ b/chrome/browser/about_flags.cc
@@ -11438,6 +11438,17 @@ const FeatureEntry kFeatureEntries[] = {
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
 #if BUILDFLAG(IS_ANDROID)
     {"new-etc1-encoder", flag_descriptions::kNewEtc1EncoderName,
      flag_descriptions::kNewEtc1EncoderDescription, kOsAndroid,
