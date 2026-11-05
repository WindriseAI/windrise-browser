diff --git a/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model_browsertest.cc b/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model_browsertest.cc
index 5fd2a91..e13f6b8 100644
--- a/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model_browsertest.cc
+++ b/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model_browsertest.cc
@@ -458,5 +458,11 @@ IN_PROC_BROWSER_TEST_F(PinnedToolbarActionsModelWithTtcDisabledBrowserTest,
       prefs::kTtcAutoPinnedMigration));
 }
 
+IN_PROC_BROWSER_TEST_F(PinnedToolbarActionsModelBrowserTest,
+                       BrowserOSDoesNotPinRemovedAssistant) {
+  model()->EnsureAlwaysPinnedActions();
+  EXPECT_FALSE(model()->Contains(kActionBrowserOSAgent));
+}
+
 // TODO(dljames): Write tests for guest and incognito mode profile that check
 // that we cannot modify the model at all.
