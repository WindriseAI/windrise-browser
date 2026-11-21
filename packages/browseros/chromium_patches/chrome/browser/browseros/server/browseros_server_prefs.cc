diff --git a/chrome/browser/browseros/server/browseros_server_prefs.cc b/chrome/browser/browseros/server/browseros_server_prefs.cc
new file mode 100644
index 0000000000000..d1829772e773f
--- /dev/null
+++ b/chrome/browser/browseros/server/browseros_server_prefs.cc
@@ -0,0 +1,112 @@
+// Copyright 2024 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/server/browseros_server_prefs.h"
+
+#include "base/values.h"
+#include "components/prefs/pref_registry_simple.h"
+#include "components/prefs/pref_service.h"
+
+namespace browseros_server {
+
+namespace {
+
+constexpr char kLegacyCDPServerPort[] = "browseros.server.cdp_port";
+constexpr char kLegacyProxyPort[] = "browseros.server.proxy_port";
+constexpr char kLegacyProxyHttpsPort[] = "browseros.server.proxy_https_port";
+constexpr char kLegacyServerPort[] = "browseros.server.server_port";
+constexpr char kLegacyAllowRemoteInMCP[] =
+    "browseros.server.allow_remote_in_mcp";
+constexpr char kLegacyRestartServerRequested[] =
+    "browseros.server.restart_requested";
+constexpr char kLegacyServerVersion[] = "browseros.server.version";
+constexpr char kLegacyMCPServerPort[] = "browseros.server.mcp_port";
+constexpr char kLegacyMCPServerEnabled[] = "browseros.server.mcp_enabled";
+
+void MigratePrefValue(PrefService* local_state,
+                      const char* old_key,
+                      const char* new_key) {
+  if (!local_state || !local_state->FindPreference(old_key) ||
+      !local_state->FindPreference(new_key)) {
+    return;
+  }
+  if (const base::Value* old_value = local_state->GetUserPrefValue(old_key)) {
+    if (!local_state->GetUserPrefValue(new_key)) {
+      local_state->Set(new_key, old_value->Clone());
+    }
+    local_state->ClearPref(old_key);
+  }
+}
+
+}  // namespace
+
+// CDP server port
+const char kCDPServerPort[] = "windrise.server.cdp_port";
+
+// Stable MCP proxy port
+const char kProxyPort[] = "windrise.server.proxy_port";
+
+// Stable HTTPS MCP proxy port
+const char kProxyHttpsPort[] = "windrise.server.proxy_https_port";
+
+// Sidecar backend server port
+const char kServerPort[] = "windrise.server.server_port";
+
+// Allow remote connections to MCP server (security setting)
+const char kAllowRemoteInMCP[] = "windrise.server.allow_remote_in_mcp";
+
+// Whether server restart has been requested (auto-reset after restart)
+const char kRestartServerRequested[] = "windrise.server.restart_requested";
+
+// Current active browseros-server version (for observability)
+const char kServerVersion[] = "windrise.server.version";
+
+// DEPRECATED prefs (kept for migration)
+const char kMCPServerPort[] = "windrise.server.mcp_port";
+const char kMCPServerEnabled[] = "windrise.server.mcp_enabled";
+
+void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
+  registry->RegisterIntegerPref(kCDPServerPort, kDefaultCDPPort);
+  registry->RegisterIntegerPref(kProxyPort, kDefaultProxyPort);
+  registry->RegisterIntegerPref(kProxyHttpsPort, kDefaultProxyHttpsPort);
+  registry->RegisterIntegerPref(kServerPort, kDefaultServerPort);
+  registry->RegisterBooleanPref(kAllowRemoteInMCP, false);
+  registry->RegisterBooleanPref(kRestartServerRequested, false);
+  registry->RegisterStringPref(kServerVersion, std::string());
+
+  // Deprecated prefs: register for migration reads
+  registry->RegisterIntegerPref(kMCPServerPort, 0);
+
+  // Legacy browseros.server.* keys registered for one-time migration.
+  registry->RegisterIntegerPref(kLegacyCDPServerPort, kDefaultCDPPort);
+  registry->RegisterIntegerPref(kLegacyProxyPort, kDefaultProxyPort);
+  registry->RegisterIntegerPref(kLegacyProxyHttpsPort, kDefaultProxyHttpsPort);
+  registry->RegisterIntegerPref(kLegacyServerPort, kDefaultServerPort);
+  registry->RegisterBooleanPref(kLegacyAllowRemoteInMCP, false);
+  registry->RegisterBooleanPref(kLegacyRestartServerRequested, false);
+  registry->RegisterStringPref(kLegacyServerVersion, std::string());
+  registry->RegisterIntegerPref(kLegacyMCPServerPort, 0);
+  registry->RegisterBooleanPref(kLegacyMCPServerEnabled, false);
+}
+
+void MigrateLocalStatePrefs(PrefService* local_state) {
+  if (!local_state) {
+    return;
+  }
+  MigratePrefValue(local_state, kLegacyCDPServerPort, kCDPServerPort);
+  MigratePrefValue(local_state, kLegacyProxyPort, kProxyPort);
+  MigratePrefValue(local_state, kLegacyProxyHttpsPort, kProxyHttpsPort);
+  MigratePrefValue(local_state, kLegacyServerPort, kServerPort);
+  MigratePrefValue(local_state, kLegacyAllowRemoteInMCP, kAllowRemoteInMCP);
+  MigratePrefValue(local_state, kLegacyRestartServerRequested,
+                   kRestartServerRequested);
+  MigratePrefValue(local_state, kLegacyServerVersion, kServerVersion);
+  MigratePrefValue(local_state, kLegacyMCPServerPort, kMCPServerPort);
+  if (local_state->FindPreference(kLegacyMCPServerEnabled) &&
+      local_state->GetUserPrefValue(kLegacyMCPServerEnabled)) {
+    local_state->ClearPref(kLegacyMCPServerEnabled);
+  }
+}
+
+}  // namespace browseros_server
