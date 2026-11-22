diff --git a/chrome/browser/extensions/api/browser_os/browser_os_api.h b/chrome/browser/extensions/api/browser_os/browser_os_api.h
new file mode 100644
index 0000000000..07b0e3fad7
--- /dev/null
+++ b/chrome/browser/extensions/api/browser_os/browser_os_api.h
@@ -0,0 +1,266 @@
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
+class WindriseGetPrefFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.getPref", WINDRISE_GETPREF)
+
+  WindriseGetPrefFunction() = default;
+
+ protected:
+  ~WindriseGetPrefFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseSetPrefFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setPref", WINDRISE_SETPREF)
+
+  WindriseSetPrefFunction() = default;
+
+ protected:
+  ~WindriseSetPrefFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseLogMetricFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.logMetric", WINDRISE_LOGMETRIC)
+
+  WindriseLogMetricFunction() = default;
+
+ protected:
+  ~WindriseLogMetricFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseGetVersionNumberFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.getVersionNumber",
+                             WINDRISE_GETVERSIONNUMBER)
+
+  WindriseGetVersionNumberFunction() = default;
+
+ protected:
+  ~WindriseGetVersionNumberFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseGetWindriseVersionNumberFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.getWindriseVersionNumber",
+                             WINDRISE_GETWINDRISEVERSIONNUMBER)
+
+  WindriseGetWindriseVersionNumberFunction() = default;
+
+ protected:
+  ~WindriseGetWindriseVersionNumberFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseChoosePathFunction : public ExtensionFunction,
+                                   public ui::SelectFileDialog::Listener {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.choosePath", WINDRISE_CHOOSEPATH)
+
+  WindriseChoosePathFunction();
+  WindriseChoosePathFunction(const WindriseChoosePathFunction&) = delete;
+  WindriseChoosePathFunction& operator=(const WindriseChoosePathFunction&) =
+      delete;
+
+  // ui::SelectFileDialog::Listener:
+  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
+  void FileSelectionCanceled() override;
+
+ protected:
+  ~WindriseChoosePathFunction() override;
+
+  ResponseAction Run() override;
+
+ private:
+  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
+};
+
+class WindriseShowToastFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.showToast", WINDRISE_SHOWTOAST)
+
+  WindriseShowToastFunction() = default;
+
+ protected:
+  ~WindriseShowToastFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseShowInfoBarFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.showInfoBar", WINDRISE_SHOWINFOBAR)
+
+  WindriseShowInfoBarFunction() = default;
+
+ protected:
+  ~WindriseShowInfoBarFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseSetAddressBarSlotFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setAddressBarSlot",
+                             WINDRISE_SETADDRESSBARSLOT)
+
+  WindriseSetAddressBarSlotFunction() = default;
+
+ protected:
+  ~WindriseSetAddressBarSlotFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseSetAiTabsFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setAiTabs", WINDRISE_SETAITABS)
+
+  WindriseSetAiTabsFunction() = default;
+
+ protected:
+  ~WindriseSetAiTabsFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseSetConnectedAisFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setConnectedAis",
+                             WINDRISE_SETCONNECTEDAIS)
+
+  WindriseSetConnectedAisFunction() = default;
+
+ protected:
+  ~WindriseSetConnectedAisFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseSetAiActivityFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setAiActivity",
+                             WINDRISE_SETAIACTIVITY)
+
+  WindriseSetAiActivityFunction() = default;
+
+ protected:
+  ~WindriseSetAiActivityFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseSetEdgeLightModeFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setEdgeLightMode",
+                             WINDRISE_SETEDGELIGHTMODE)
+
+  WindriseSetEdgeLightModeFunction() = default;
+
+ protected:
+  ~WindriseSetEdgeLightModeFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindrisePulseAiActivityFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.pulseAiActivity",
+                             WINDRISE_PULSEAIACTIVITY)
+
+  WindrisePulseAiActivityFunction() = default;
+
+ protected:
+  ~WindrisePulseAiActivityFunction() override = default;
+
+  ResponseAction Run() override;
+};
+
+class WindriseGetDefaultSearchEngineFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.getDefaultSearchEngine",
+                             WINDRISE_GETDEFAULTSEARCHENGINE)
+
+ protected:
+  ~WindriseGetDefaultSearchEngineFunction() override = default;
+  ResponseAction Run() override;
+
+ private:
+  ResponseValue BuildResponse();
+  void OnLoaded();
+  std::string query_;
+  base::CallbackListSubscription loaded_subscription_;
+};
+
+class WindriseSetDefaultSearchEngineFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setDefaultSearchEngine",
+                             WINDRISE_SETDEFAULTSEARCHENGINE)
+
+ protected:
+  ~WindriseSetDefaultSearchEngineFunction() override = default;
+  ResponseAction Run() override;
+
+ private:
+  ResponseValue ApplySelection();
+  void OnLoaded();
+  int prepopulate_id_ = 0;
+  base::CallbackListSubscription loaded_subscription_;
+};
+
+class WindriseSetNewTabAsHomePageFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setNewTabAsHomePage",
+                             WINDRISE_SETNEWTABASHOMEPAGE)
+
+ protected:
+  ~WindriseSetNewTabAsHomePageFunction() override = default;
+  ResponseAction Run() override;
+};
+
+class WindriseGetColorSchemeFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.getColorScheme",
+                             WINDRISE_GETCOLORSCHEME)
+
+ protected:
+  ~WindriseGetColorSchemeFunction() override = default;
+  ResponseAction Run() override;
+};
+
+class WindriseSetColorSchemeFunction : public ExtensionFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("windrise.setColorScheme",
+                             WINDRISE_SETCOLORSCHEME)
+
+ protected:
+  ~WindriseSetColorSchemeFunction() override = default;
+  ResponseAction Run() override;
+};
+
+}  // namespace extensions::api
+
+#endif  // CHROME_BROWSER_EXTENSIONS_API_BROWSER_OS_BROWSER_OS_API_H_
