diff --git a/chrome/browser/ui/color/chrome_color_id.h b/chrome/browser/ui/color/chrome_color_id.h
index 6129096713..2f7fbcc73f 100644
--- a/chrome/browser/ui/color/chrome_color_id.h
+++ b/chrome/browser/ui/color/chrome_color_id.h
@@ -1194,6 +1194,24 @@
   /* Window control button background colors. */ \
   E_CPONLY(kColorWindowControlButtonBackgroundActive) \
   E_CPONLY(kColorWindowControlButtonBackgroundInactive) \
+  /* Windrise colors. Values come from the windrise color mixer, which reads */ \
+  /* browseros.windrise.theme.* profile prefs (see windrise_color_mixer.cc). */ \
+  E_CPONLY(kColorWindriseCardBackground) \
+  E_CPONLY(kColorWindrisePanelBackground) \
+  E_CPONLY(kColorWindriseTabActiveBackground) \
+  E_CPONLY(kColorWindriseTabInactiveBackground) \
+  E_CPONLY(kColorWindriseTileBackground) \
+  E_CPONLY(kColorWindriseDivider) \
+  E_CPONLY(kColorWindriseWebContentsGap) \
+  E_CPONLY(kColorWindriseSectionHeaderText) \
+  E_CPONLY(kColorWindriseSectionDivider) \
+  E_CPONLY(kColorWindriseSlotChipBackground) \
+  E_CPONLY(kColorWindriseSlotChipForeground) \
+  E_CPONLY(kColorWindriseAiCardBackground) \
+  E_CPONLY(kColorWindriseAiPanelBackground) \
+  E_CPONLY(kColorWindriseAiPanelButtonBackground) \
+  E_CPONLY(kColorWindriseAiCardBackgroundTop) \
+  E_CPONLY(kColorWindriseAiCardBackgroundBottom) \
 
 #if BUILDFLAG(IS_CHROMEOS)
 #define CHROME_PLATFORM_SPECIFIC_COLOR_IDS \
