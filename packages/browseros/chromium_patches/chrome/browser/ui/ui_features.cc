diff --git a/chrome/browser/ui/ui_features.cc b/chrome/browser/ui/ui_features.cc
index 7a0e331dedf4563a6be3f5a9e4874d0293abce1c..b744acc04ec2e5dce858eb70457b555e6392a118 100644
--- a/chrome/browser/ui/ui_features.cc
+++ b/chrome/browser/ui/ui_features.cc
@@ -157,7 +157,7 @@ BASE_FEATURE(kExtensionsPinnedByDefault, base::FEATURE_DISABLED_BY_DEFAULT);
 #if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
 // Shows an infobar on PDFs offering to become the default PDF viewer if Chrome
 // isn't the default already.
-BASE_FEATURE(kPdfInfoBar, base::FEATURE_ENABLED_BY_DEFAULT);
+BASE_FEATURE(kPdfInfoBar, base::FEATURE_DISABLED_BY_DEFAULT);
 
 BASE_FEATURE(kSeparateDefaultAndPinPrompt,
 #if BUILDFLAG(IS_WIN)
@@ -254,6 +254,12 @@ BASE_FEATURE_PARAM(int,
                    "max_distance_threshold",
                    20);
 
+// Off by default: the panel is an opt-in in-browser AI surface. The same
+// feature also gates its command, toolbar action and side-panel entry.
+BASE_FEATURE(kThirdPartyLlmPanel,
+             "ThirdPartyLlmPanel",
+             base::FEATURE_DISABLED_BY_DEFAULT);
+
 BASE_FEATURE(kTabDuplicateMetrics, base::FEATURE_ENABLED_BY_DEFAULT);
 
 // Enables collapsing a tab group programmatically during a drag.
