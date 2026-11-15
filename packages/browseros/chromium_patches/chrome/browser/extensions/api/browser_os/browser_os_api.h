diff --git a/chrome/browser/extensions/api/browser_os/browser_os_api.h b/chrome/browser/extensions/api/browser_os/browser_os_api.h
new file mode 100644
index 0000000000..07b0e3fad7
--- /dev/null
+++ b/chrome/browser/extensions/api/browser_os/browser_os_api.h
@@ -0,0 +1,220 @@
+// Copyright 2024 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_EXTENSIONS_API_BROWSER_OS_BROWSER_OS_API_H_
+#define CHROME_BROWSER_EXTENSIONS_API_BROWSER_OS_BROWSER_OS_API_H_
+
+#include <string>
+
+#include "base/callback_list.h"
+#include "extensions/browser/extension_function.h"
+#include "ui/shell_dialogs/select_file_dialog.h"
+
+namespace extensions::api {
+
+class BrowserOSGetPrefFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.getPref", BROWSER_OS_GETPREF)
+
+  BrowserOSGetPrefFunction() = default;
+
+ protected:
+  ~BrowserOSGetPrefFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSSetPrefFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.setPref", BROWSER_OS_SETPREF)
+
+  BrowserOSSetPrefFunction() = default;
+
+ protected:
+  ~BrowserOSSetPrefFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSLogMetricFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.logMetric", BROWSER_OS_LOGMETRIC)
+
+  BrowserOSLogMetricFunction() = default;
+
+ protected:
+  ~BrowserOSLogMetricFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSGetVersionNumberFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.getVersionNumber",
+                             BROWSER_OS_GETVERSIONNUMBER)
+
+  BrowserOSGetVersionNumberFunction() = default;
+
+ protected:
+  ~BrowserOSGetVersionNumberFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSGetBrowserosVersionNumberFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.getBrowserosVersionNumber",
+                             BROWSER_OS_GETBROWSEROSVERSIONNUMBER)
+
+  BrowserOSGetBrowserosVersionNumberFunction() = default;
+
+ protected:
+  ~BrowserOSGetBrowserosVersionNumberFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSChoosePathFunction : public ExtensionFunction,
+                                    public ui::SelectFileDialog::Listener {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.choosePath", BROWSER_OS_CHOOSEPATH)
+
+  BrowserOSChoosePathFunction();
+  BrowserOSChoosePathFunction(const BrowserOSChoosePathFunction&) = delete;
+  BrowserOSChoosePathFunction& operator=(const BrowserOSChoosePathFunction&) =
+      delete;
+
+  // ui::SelectFileDialog::Listener:
+  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
+  void FileSelectionCanceled() override;
+
+ protected:
+  ~BrowserOSChoosePathFunction() override;
+
+  ResponseAction Run() override;
+
+ private:
+  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
+};
+
+class BrowserOSShowToastFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.showToast", BROWSER_OS_SHOWTOAST)
+
+  BrowserOSShowToastFunction() = default;
+
+ protected:
+  ~BrowserOSShowToastFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSShowInfoBarFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.showInfoBar", BROWSER_OS_SHOWINFOBAR)
+
+  BrowserOSShowInfoBarFunction() = default;
+
+ protected:
+  ~BrowserOSShowInfoBarFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSSetAddressBarSlotFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.setAddressBarSlot",
+                             BROWSER_OS_SETADDRESSBARSLOT)
+
+  BrowserOSSetAddressBarSlotFunction() = default;
+
+ protected:
+  ~BrowserOSSetAddressBarSlotFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSSetAiTabsFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.setAiTabs", BROWSER_OS_SETAITABS)
+
+  BrowserOSSetAiTabsFunction() = default;
+
+ protected:
+  ~BrowserOSSetAiTabsFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSSetConnectedAisFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.setConnectedAis",
+                             BROWSER_OS_SETCONNECTEDAIS)
+
+  BrowserOSSetConnectedAisFunction() = default;
+
+ protected:
+  ~BrowserOSSetConnectedAisFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSSetAiActivityFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.setAiActivity",
+                             BROWSER_OS_SETAIACTIVITY)
+
+  BrowserOSSetAiActivityFunction() = default;
+
+ protected:
+  ~BrowserOSSetAiActivityFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSSetEdgeLightModeFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.setEdgeLightMode",
+                             BROWSER_OS_SETEDGELIGHTMODE)
+
+  BrowserOSSetEdgeLightModeFunction() = default;
+
+ protected:
+  ~BrowserOSSetEdgeLightModeFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSPulseAiActivityFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.pulseAiActivity",
+                             BROWSER_OS_PULSEAIACTIVITY)
+
+  BrowserOSPulseAiActivityFunction() = default;
+
+ protected:
+  ~BrowserOSPulseAiActivityFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class BrowserOSGetDefaultSearchEngineFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("browserOS.getDefaultSearchEngine",
+                             BROWSER_OS_GETDEFAULTSEARCHENGINE)
+
+ protected:
+  ~BrowserOSGetDefaultSearchEngineFunction() override = default;
+  ResponseAction Run() override;
+
+ private:
+  ResponseValue BuildResponse();
+  void OnLoaded();
+  std::string query_;
+  base::CallbackListSubscription loaded_subscription_;
+};
+
+}  // namespace extensions::api
+
+#endif  // CHROME_BROWSER_EXTENSIONS_API_BROWSER_OS_BROWSER_OS_API_H_
