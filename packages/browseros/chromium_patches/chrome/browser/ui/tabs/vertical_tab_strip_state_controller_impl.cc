diff --git a/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.cc b/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.cc
index 4eb9220..ae497de 100644
--- a/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.cc
+++ b/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.cc
@@ -12,6 +12,7 @@
 #include "base/strings/string_number_conversions.h"
 #include "base/strings/to_string.h"
 #include "base/timer/timer.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
 #include "chrome/app/vector_icons/vector_icons.h"
 #include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/sessions/session_service.h"
@@ -55,6 +56,8 @@ VerticalTabStripStateControllerImpl::VerticalTabStripStateControllerImpl(
       session_service_(session_service),
       session_id_(session_id),
       browser_window_(browser_window) {
+  browseros::SyncVerticalTabsPref(pref_service_);
+
   pref_change_registrar_.Init(pref_service_);
 
   is_vertical_tabs_enabled_ =
@@ -74,6 +77,28 @@ VerticalTabStripStateControllerImpl::VerticalTabStripStateControllerImpl(
           &VerticalTabStripStateControllerImpl::OnExpandOnHoverEnabledChanged,
           base::Unretained(this)));
 
+  pref_change_registrar_.Add(
+      browseros::prefs::kVerticalTabsEnabled,
+      base::BindRepeating(
+          [](PrefService* ps) {
+            ps->SetBoolean(
+                prefs::kVerticalTabsEnabled,
+                ps->GetBoolean(browseros::prefs::kVerticalTabsEnabled));
+          },
+          base::Unretained(pref_service_)));
+
+  // Windrise starts collapsed. Only apply this while the persisted state is
+  // still the upstream default: once the user expands or collapses the strip,
+  // Chromium writes that choice and it must win on later launches.
+  const PrefService::Preference* collapsed_pref =
+      pref_service_->FindPreference(prefs::kVerticalTabsCollapsedState);
+  if (restored_state_collapsed.has_value() &&
+      !restored_state_collapsed.value() && collapsed_pref &&
+      collapsed_pref->IsDefaultValue() &&
+      browseros::ShouldStartVerticalTabsCollapsed(pref_service_)) {
+    restored_state_collapsed = true;
+  }
+
   if (restored_state_collapsed.has_value()) {
     SetCollapsed(restored_state_collapsed.value());
   }
