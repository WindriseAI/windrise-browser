diff --git a/chrome/browser/ui/browser_command_controller.cc b/chrome/browser/ui/browser_command_controller.cc
index 3741ba7..c01293c 100644
--- a/chrome/browser/ui/browser_command_controller.cc
+++ b/chrome/browser/ui/browser_command_controller.cc
@@ -1751,7 +1751,13 @@ void BrowserCommandController::InitCommandState() {
   UpdateTabRestoreCommandState();
   command_updater_->UpdateCommandEnabled(IDC_EXIT, true);
   command_updater_->UpdateCommandEnabled(IDC_NAME_WINDOW, true);
-  command_updater_->UpdateCommandEnabled(IDC_TOGGLE_VERTICAL_TABS, true);
+  // BrowserOS: off while the layout pref can't change, e.g. when
+  // --disable-browseros-features=VerticalTabs forces it. Forced values hold
+  // for the whole run, so checking once per window is enough.
+  command_updater_->UpdateCommandEnabled(
+      IDC_TOGGLE_VERTICAL_TABS,
+      profile()->GetPrefs()->IsUserModifiablePreference(
+          prefs::kVerticalTabsEnabled));
   command_updater_->UpdateCommandEnabled(IDC_VERTICAL_TABS_SEND_FEEDBACK, true);
   command_updater_->UpdateCommandEnabled(
       IDC_TOGGLE_VERTICAL_TABS_EXPAND_ON_HOVER, true);
