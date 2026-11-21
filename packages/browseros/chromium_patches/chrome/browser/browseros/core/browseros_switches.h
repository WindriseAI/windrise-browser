diff --git a/chrome/browser/browseros/core/browseros_switches.h b/chrome/browser/browseros/core/browseros_switches.h
new file mode 100644
index 0000000000000..1a93529125cbe
--- /dev/null
+++ b/chrome/browser/browseros/core/browseros_switches.h
@@ -0,0 +1,31 @@
+// Copyright 2024 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_SWITCHES_H_
+#define CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_SWITCHES_H_
+
+namespace browseros {
+
+inline constexpr char kEnableServer[] = "enable-windrise-server";
+inline constexpr char kDisableServer[] = "disable-windrise-server";
+inline constexpr char kLegacyDisableServer[] = "disable-browseros-server";
+inline constexpr char kDisableServerUpdater[] =
+    "disable-windrise-server-updater";
+inline constexpr char kLegacyDisableServerUpdater[] =
+    "disable-browseros-server-updater";
+inline constexpr char kServerAppcastUrl[] = "windrise-server-appcast-url";
+inline constexpr char kServerResourcesDir[] = "windrise-server-resources-dir";
+inline constexpr char kCDPPort[] = "windrise-cdp-port";
+inline constexpr char kProxyPort[] = "windrise-proxy-port";
+inline constexpr char kServerPort[] = "windrise-server-port";
+inline constexpr char kDisableExtensions[] = "disable-windrise-extensions";
+inline constexpr char kLegacyDisableExtensions[] =
+    "disable-browseros-extensions";
+inline constexpr char kDisableUrlOverrides[] =
+    "windrise-disable-url-overrides";
+inline constexpr char kBrowserOSProduct[] = "windrise-product";
+inline constexpr char kDockIcon[] = "windrise-dock-icon";
+}  // namespace browseros
+
+#endif  // CHROME_BROWSER_BROWSEROS_CORE_BROWSEROS_SWITCHES_H_
