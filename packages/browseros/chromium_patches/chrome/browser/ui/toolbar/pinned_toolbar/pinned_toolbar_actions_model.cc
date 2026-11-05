diff --git a/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.cc b/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.cc
index dcd99a8..5f7548c 100644
--- a/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.cc
+++ b/chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.cc
@@ -16,6 +16,8 @@
 #include "base/observer_list.h"
 #include "base/strings/strcat.h"
 #include "base/values.h"
+#include "chrome/browser/browseros/core/browseros_action_utils.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
 #include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/ttc/core/features.h"
 #include "chrome/browser/ui/actions/chrome_action_id.h"
@@ -38,8 +40,16 @@ PinnedToolbarActionsModel::PinnedToolbarActionsModel(Profile* profile)
       base::BindRepeating(&PinnedToolbarActionsModel::UpdatePinnedActionIds,
                           base::Unretained(this)));
 
+  // Observe the toolbar label pref for reactive updates.
+  pref_change_registrar_.Add(
+      browseros::prefs::kShowToolbarLabels,
+      base::BindRepeating(
+          &PinnedToolbarActionsModel::OnBrowserOSLabelsPrefChanged,
+          base::Unretained(this)));
+
   // Initialize the model with the current state of the kPinnedActions pref.
   UpdatePinnedActionIds();
+  EnsureAlwaysPinnedActions();
 }
 
 PinnedToolbarActionsModel::~PinnedToolbarActionsModel() = default;
@@ -238,8 +248,11 @@ void PinnedToolbarActionsModel::MaybeMigrateExistingPinnedStates() {
   if (!CanUpdate()) {
     return;
   }
+  // Chrome Labs is no longer automatically pinned for new profiles
+  // We keep this migration complete check to not affect users who already have
+  // it
   if (!pref_service_->GetBoolean(prefs::kPinnedChromeLabsMigrationComplete)) {
-    UpdatePinnedState(kActionShowChromeLabs, true);
+    // UpdatePinnedState(kActionShowChromeLabs, true);  // No longer auto-pin
     pref_service_->SetBoolean(prefs::kPinnedChromeLabsMigrationComplete, true);
   }
   if (!pref_service_->GetBoolean(prefs::kPinnedCastMigrationComplete)) {
@@ -263,6 +276,25 @@ void PinnedToolbarActionsModel::MaybeMigrateExistingPinnedStates() {
   }
 }
 
+void PinnedToolbarActionsModel::EnsureAlwaysPinnedActions() {
+  // Only update if we're allowed to (not incognito/guest profiles).
+  if (!CanUpdate()) {
+    return;
+  }
+
+  // Older builds pinned the agent extension's side-panel action. Remove that
+  // stale entry when a profile upgrades to a build without the extension.
+  const auto agent_extension_action_id = actions::ActionIdMap::StringToActionId(
+      SidePanelEntryKey(SidePanelEntryId::kExtension,
+                        browseros::kAgentExtensionId)
+          .ToString());
+  if (agent_extension_action_id && Contains(*agent_extension_action_id)) {
+    UpdatePinnedState(*agent_extension_action_id, false);
+  }
+
+  // Note: Extension pinning is handled by ExtensionSidePanelManager
+}
+
 const std::vector<actions::ActionId>&
 PinnedToolbarActionsModel::PinnedActionIds() const {
   return pinned_action_ids_;
@@ -281,3 +313,10 @@ void PinnedToolbarActionsModel::UpdatePref(
     list_of_values.Append(id_string.value());
   }
 }
+
+void PinnedToolbarActionsModel::OnBrowserOSLabelsPrefChanged() {
+  // Notify observers so buttons can refresh their labels.
+  for (Observer& observer : observers_) {
+    observer.OnLabelsVisibilityChanged();
+  }
+}
