diff --git a/chrome/browser/ui/toolbar/toolbar_pref_names.cc b/chrome/browser/ui/toolbar/toolbar_pref_names.cc
index d93b933..ebac95c 100644
--- a/chrome/browser/ui/toolbar/toolbar_pref_names.cc
+++ b/chrome/browser/ui/toolbar/toolbar_pref_names.cc
@@ -14,23 +14,6 @@ namespace toolbar {
 
 void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
   base::ListValue default_pinned_actions;
-  const std::optional<std::string>& chrome_labs_action =
-      actions::ActionIdMap::ActionIdToString(kActionShowChromeLabs);
-  // ActionIdToStringMappings are not initialized in unit tests, therefore will
-  // not have a value. In the normal case, the action should always have a
-  // value.
-  if (chrome_labs_action.has_value()) {
-    default_pinned_actions.Append(chrome_labs_action.value());
-  }
-
-  if (base::FeatureList::IsEnabled(
-          features::kTabsFromOtherDevicesSidePanelPinnedByDefault)) {
-    const std::optional<std::string>& tabs_from_other_devices_action =
-        actions::ActionIdMap::ActionIdToString(
-            kActionSidePanelShowTabsFromOtherDevices);
-    if (tabs_from_other_devices_action.has_value()) {
-      default_pinned_actions.Append(tabs_from_other_devices_action.value());
-    }
-  }
+  // Windrise: do not auto-pin Chrome Labs or Side Panel actions by default.
 
   if (base::FeatureList::IsEnabled(ttc::kTtc)) {
