diff --git a/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model_browsertest.cc b/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model_browsertest.cc
index bedf19f6dc172877221249e53ecd736563c7fe99..5a72cce2a4724a9df8eb2d8233a2665fa53257a1 100644
--- a/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model_browsertest.cc
+++ b/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model_browsertest.cc
@@ -397,5 +397,11 @@ IN_PROC_BROWSER_TEST_F(
   EXPECT_FALSE(model()->Contains(kActionSidePanelShowTabsFromOtherDevices));
 }
 
+IN_PROC_BROWSER_TEST_F(PinnedToolbarActionsModelBrowserTest,
+                       BrowserOSDoesNotPinRemovedAssistant) {
+  model()->EnsureAlwaysPinnedActions();
+  EXPECT_FALSE(model()->Contains(kActionBrowserOSAgent));
+}
+
 // TODO(dljames): Write tests for guest and incognito mode profile that check
 // that we cannot modify the model at all.
