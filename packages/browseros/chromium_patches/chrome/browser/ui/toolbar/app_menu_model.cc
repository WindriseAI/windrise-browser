diff --git a/chrome/browser/ui/toolbar/app_menu_model.cc b/chrome/browser/ui/toolbar/app_menu_model.cc
--- a/chrome/browser/ui/toolbar/app_menu_model.cc
+++ b/chrome/browser/ui/toolbar/app_menu_model.cc
@@ -371,9 +371,6 @@
         case syncer::SyncService::UserActionableError::kNeedsClientUpgrade:
-          info.command_id = IDC_UPGRADE_DIALOG;
-          info.has_actionable_error = true;
-          info.icon = &(features::IsRoundedIconsEnabled()
-                            ? vector_icons::kErrorIcon
-                            : vector_icons::kErrorOutlineOldIcon);
-          break;
+          // Browser package updates belong to Windrise Bridge; do not expose
+          // the Chrome download/update command through a sync error either.
+          return info;
         case syncer::SyncService::UserActionableError::kNeedsPassphrase:
           info.command_id = IDC_SHOW_SYNC_PASSPHRASE_DIALOG;
