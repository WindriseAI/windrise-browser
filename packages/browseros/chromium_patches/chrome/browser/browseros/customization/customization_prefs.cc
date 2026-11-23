diff --git a/chrome/browser/browseros/customization/customization_prefs.cc b/chrome/browser/browseros/customization/customization_prefs.cc
new file mode 100644
index 0000000000000000000000000000000000000000..4908509187b468adbcdfb7e565b600404b8993ff
--- /dev/null
+++ b/chrome/browser/browseros/customization/customization_prefs.cc
@@ -0,0 +1,55 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/customization/customization_prefs.h"
+
+#include "base/values.h"
+#include "chrome/browser/browseros/customization/customization_pref_names.h"
+#include "components/pref_registry/pref_registry_syncable.h"
+#include "components/prefs/pref_service.h"
+
+namespace browseros {
+
+namespace {
+
+constexpr char kLegacyShowAvatarButton[] = "browseros.show_avatar_button";
+constexpr char kLegacyShowExtensionsToolbar[] =
+    "browseros.show_extensions_toolbar";
+
+void MigratePrefValue(PrefService* pref_service,
+                      const char* old_key,
+                      const char* new_key) {
+  if (!pref_service || !pref_service->FindPreference(old_key) ||
+      !pref_service->FindPreference(new_key)) {
+    return;
+  }
+  if (const base::Value* old_value = pref_service->GetUserPrefValue(old_key)) {
+    if (!pref_service->GetUserPrefValue(new_key)) {
+      pref_service->Set(new_key, old_value->Clone());
+    }
+    pref_service->ClearPref(old_key);
+  }
+}
+
+}  // namespace
+
+void RegisterCustomizationProfilePrefs(
+    user_prefs::PrefRegistrySyncable* registry) {
+  // Windrise: identity isolation is handled by Grids; hide the upstream
+  // profile avatar toolbar button by default.
+  registry->RegisterBooleanPref(prefs::kShowAvatarButton, false);
+  registry->RegisterBooleanPref(prefs::kShowExtensionsToolbar, true);
+  // Legacy keys registered for one-time migration.
+  registry->RegisterBooleanPref(kLegacyShowAvatarButton, false);
+  registry->RegisterBooleanPref(kLegacyShowExtensionsToolbar, true);
+}
+
+void MigrateCustomizationProfilePrefs(PrefService* pref_service) {
+  MigratePrefValue(pref_service, kLegacyShowAvatarButton,
+                   prefs::kShowAvatarButton);
+  MigratePrefValue(pref_service, kLegacyShowExtensionsToolbar,
+                   prefs::kShowExtensionsToolbar);
+}
+
+}  // namespace browseros
