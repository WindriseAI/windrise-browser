diff --git a/chrome/browser/browseros/onboarding/browseros_onboarding_prefs.cc b/chrome/browser/browseros/onboarding/browseros_onboarding_prefs.cc
new file mode 100644
index 0000000000000000000000000000000000000000..7cdf1c58ed69aa67d0ed7e8ce8244ba64bdc7b96
--- /dev/null
+++ b/chrome/browser/browseros/onboarding/browseros_onboarding_prefs.cc
@@ -0,0 +1,84 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/onboarding/browseros_onboarding_prefs.h"
+
+#include "base/files/file_path.h"
+#include "base/files/file_util.h"
+#include "base/threading/thread_restrictions.h"
+#include "chrome/browser/browser_process.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
+#include "chrome/browser/profiles/profile.h"
+#include "chrome/common/chrome_constants.h"
+#include "chrome/common/pref_names.h"
+#include "components/prefs/pref_service.h"
+
+namespace browseros::onboarding {
+
+bool HasDesktopOnboardingMarker(const base::FilePath& user_data_dir) {
+  if (user_data_dir.empty()) {
+    return false;
+  }
+  base::ScopedAllowBlocking allow_blocking;
+  return base::PathExists(
+      user_data_dir.AppendASCII(kDesktopOnboardingDoneMarker));
+}
+
+bool ConsumeDesktopOnboardingMarker(Profile* profile) {
+  if (!profile || !profile->IsRegularProfile() || profile->IsOffTheRecord()) {
+    return false;
+  }
+
+  if (profile->GetBaseName() !=
+      base::FilePath().AppendASCII(chrome::kInitialProfile)) {
+    return false;
+  }
+
+  if (profile->GetPrefs()->GetBoolean(browseros::prefs::kOnboardingCompleted)) {
+    return false;
+  }
+
+  if (!HasDesktopOnboardingMarker(profile->GetPath().DirName())) {
+    return false;
+  }
+
+  MarkCompleted(profile);
+  return true;
+}
+
+bool ShouldShow(Profile* profile) {
+  if (!profile || !profile->IsRegularProfile() || profile->IsOffTheRecord()) {
+    return false;
+  }
+
+  if (profile->GetBaseName() !=
+      base::FilePath().AppendASCII(chrome::kInitialProfile)) {
+    return false;
+  }
+
+  if (profile->GetPrefs()->GetBoolean(browseros::prefs::kOnboardingCompleted)) {
+    return false;
+  }
+
+  return !HasDesktopOnboardingMarker(profile->GetPath().DirName());
+}
+
+void MarkCompleted(Profile* profile) {
+  if (!profile || !profile->IsRegularProfile()) {
+    return;
+  }
+
+  PrefService* prefs = profile->GetPrefs();
+  prefs->SetBoolean(browseros::prefs::kOnboardingCompleted, true);
+  prefs->CommitPendingWrite();
+}
+
+void NeutralizeUpstreamFirstRun() {
+  // BrowserOS owns first-run policy whether it skips or replaces onboarding.
+  if (PrefService* local_state = g_browser_process->local_state()) {
+    local_state->SetBoolean(::prefs::kFirstRunFinished, true);
+  }
+}
+
+}  // namespace browseros::onboarding
