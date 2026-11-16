diff --git a/chrome/browser/ui/browser_command_controller.cc b/chrome/browser/ui/browser_command_controller.cc
index e12e43ef64..e25f9c6b54 100644
--- a/chrome/browser/ui/browser_command_controller.cc
+++ b/chrome/browser/ui/browser_command_controller.cc
@@ -1847,7 +1847,13 @@ void BrowserCommandController::InitCommandState() {
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
@@ -2067,8 +2073,8 @@ void BrowserCommandController::InitCommandState() {
   command_updater_->UpdateCommandEnabled(IDC_TAB_SCROLL_BUTTONS_TOGGLE_PIN,
                                          supports_tabs);
 
-  // These are always enabled; the menu determines their menu item visibility.
-  command_updater_->UpdateCommandEnabled(IDC_UPGRADE_DIALOG, true);
+  // Browser package updates are owned by Windrise Bridge.
+  command_updater_->UpdateCommandEnabled(IDC_UPGRADE_DIALOG, false);
 #if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
   command_updater_->UpdateCommandEnabled(IDC_SCHEDULED_RESTART_DIALOG, true);
 #endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
