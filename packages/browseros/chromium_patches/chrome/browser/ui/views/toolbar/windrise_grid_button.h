diff --git a/chrome/browser/ui/views/toolbar/windrise_grid_button.h b/chrome/browser/ui/views/toolbar/windrise_grid_button.h
new file mode 100644
index 0000000000..429b72667f
--- /dev/null
+++ b/chrome/browser/ui/views/toolbar/windrise_grid_button.h
@@ -0,0 +1,42 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_WINDRISE_GRID_BUTTON_H_
+#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_WINDRISE_GRID_BUTTON_H_
+
+#include "base/memory/raw_ptr.h"
+#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+
+class BrowserWindowInterface;
+
+// 风桥工具栏「格子」按钮（方案 N8）。位置：地址栏之后、扩展按钮之前。
+// 外观（高、圆角、内边距、图标间距、字号、底色）都从 browseros.windrise.layout.*
+// 与 browseros.windrise.theme.* 偏好取，改偏好并在运行时重绘即可，不用重编。
+// 点击时不自己做任何事，只把 browserOS.onGridButtonClicked 广播给扩展，
+// 由扩展开格子模式（同地址栏插槽 browserOS.onAddressBarSlotClicked 的做法）。
+class WindriseGridButton : public ToolbarButton {
+  METADATA_HEADER(WindriseGridButton, ToolbarButton)
+
+ public:
+  explicit WindriseGridButton(BrowserWindowInterface* browser);
+  WindriseGridButton(const WindriseGridButton&) = delete;
+  WindriseGridButton& operator=(const WindriseGridButton&) = delete;
+  ~WindriseGridButton() override;
+
+  // views::View:
+  void OnThemeChanged() override;
+
+  // ToolbarButton:
+  int GetRoundedCornerRadius() const override;
+
+ private:
+  // 把偏好里的外观数值套到按钮上。偏好变化后由 OnThemeChanged() 重新调用。
+  void ApplyStyleFromPrefs();
+
+  void NotifyExtensionGridClicked();
+
+  raw_ptr<BrowserWindowInterface> browser_;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_WINDRISE_GRID_BUTTON_H_
