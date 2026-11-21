diff --git a/chrome/browser/extensions/api/side_panel/side_panel_api.h b/chrome/browser/extensions/api/side_panel/side_panel_api.h
index 72a88888eb9fc..3f0779a57b615 100644
--- a/chrome/browser/extensions/api/side_panel/side_panel_api.h
+++ b/chrome/browser/extensions/api/side_panel/side_panel_api.h
@@ -115,6 +115,36 @@ class SidePanelCloseFunction : public SidePanelApiFunction {
   ResponseAction RunFunction() override;
 };
 
+class SidePanelWindriseToggleFunction : public SidePanelApiFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("sidePanel.windriseToggle",
+                             SIDEPANEL_WINDRISETOGGLE)
+  SidePanelWindriseToggleFunction() = default;
+  SidePanelWindriseToggleFunction(const SidePanelWindriseToggleFunction&) =
+      delete;
+  SidePanelWindriseToggleFunction& operator=(
+      const SidePanelWindriseToggleFunction&) = delete;
+
+ private:
+  ~SidePanelWindriseToggleFunction() override = default;
+  ResponseAction RunFunction() override;
+};
+
+class SidePanelWindriseIsOpenFunction : public SidePanelApiFunction {
+ public:
+  DECLARE_EXTENSION_FUNCTION("sidePanel.windriseIsOpen",
+                             SIDEPANEL_WINDRISEISOPEN)
+  SidePanelWindriseIsOpenFunction() = default;
+  SidePanelWindriseIsOpenFunction(const SidePanelWindriseIsOpenFunction&) =
+      delete;
+  SidePanelWindriseIsOpenFunction& operator=(
+      const SidePanelWindriseIsOpenFunction&) = delete;
+
+ private:
+  ~SidePanelWindriseIsOpenFunction() override = default;
+  ResponseAction RunFunction() override;
+};
+
 }  // namespace extensions
 
 #endif  // CHROME_BROWSER_EXTENSIONS_API_SIDE_PANEL_SIDE_PANEL_API_H_
