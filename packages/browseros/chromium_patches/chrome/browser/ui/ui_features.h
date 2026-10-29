diff --git a/chrome/browser/ui/ui_features.h b/chrome/browser/ui/ui_features.h
index f7e708c05428553826e4cded856b9806178f0173..8ee0776f3454dbcf5fdce1dc14f388e88a108fc4 100644
--- a/chrome/browser/ui/ui_features.h
+++ b/chrome/browser/ui/ui_features.h
@@ -204,6 +204,9 @@ BASE_DECLARE_FEATURE_PARAM(base::TimeDelta, kSplitViewDragAndDropMaxDelay);
 BASE_DECLARE_FEATURE_PARAM(int, kSplitViewDragAndDropMinDistanceThreshold);
 BASE_DECLARE_FEATURE_PARAM(int, kSplitViewDragAndDropMaxDistanceThreshold);
 
+// BrowserOS: feature declarations
+BASE_DECLARE_FEATURE(kThirdPartyLlmPanel);
+
 BASE_DECLARE_FEATURE(kTabDuplicateMetrics);
 
 BASE_DECLARE_FEATURE(kCollapseTabGroupDuringDrag);
