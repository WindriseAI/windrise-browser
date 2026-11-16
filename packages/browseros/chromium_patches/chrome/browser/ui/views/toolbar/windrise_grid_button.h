diff --git a/chrome/browser/ui/views/toolbar/windrise_grid_button.h b/chrome/browser/ui/views/toolbar/windrise_grid_button.h
new file mode 100644
index 0000000000..1b8830193b
--- /dev/null
+++ b/chrome/browser/ui/views/toolbar/windrise_grid_button.h
@@ -0,0 +1,61 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_WINDRISE_GRID_BUTTON_H_
+#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_WINDRISE_GRID_BUTTON_H_
+
+#include "base/memory/raw_ptr.h"
+#include "base/scoped_observation.h"
+#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/views/widget/widget.h"
+#include "ui/views/widget/widget_observer.h"
+
+class BrowserWindowInterface;
+
+// 风桥工具栏「格子」按钮（方案 N8）。位置：地址栏之后、扩展按钮之前。
+// 外观（高、圆角、内边距、图标间距、字号、底色）都从 browseros.windrise.layout.*
+// 与 browseros.windrise.theme.* 偏好取，改偏好并在运行时重绘即可，不用重编。
+// 点击时不自己做任何事，只把 browserOS.onGridButtonClicked 广播给扩展，
+// 由扩展开格子模式（同地址栏插槽 browserOS.onAddressBarSlotClicked 的做法）。
+// 窄窗（窗口宽 < 900，和侧栏自动收起同一个阈值 browseros::windrise::
+// IsNarrowWindow）时收成只剩图标的 32 见方圆钮，宽回来恢复「图标 + 格子」；
+// 提示和读屏名字一直是「格子」。切换不做动画（拖窗口边是连续操作）。
+class WindriseGridButton : public ToolbarButton, public views::WidgetObserver {
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
+  void AddedToWidget() override;
+  void RemovedFromWidget() override;
+
+  // views::WidgetObserver:
+  void OnWidgetBoundsChanged(views::Widget* widget,
+                             const gfx::Rect& new_bounds) override;
+  void OnWidgetDestroying(views::Widget* widget) override;
+
+  // ToolbarButton:
+  int GetRoundedCornerRadius() const override;
+
+ private:
+  // 把偏好里的外观数值套到按钮上。偏好变化后由 OnThemeChanged() 重新调用。
+  void ApplyStyleFromPrefs();
+
+  void NotifyExtensionGridClicked();
+  // 按窗口宽判断要不要收成只剩图标；变了才重排。
+  void UpdateCompact(int window_width);
+
+  raw_ptr<BrowserWindowInterface> browser_;
+  // 窄窗：只剩图标。
+  bool compact_ = false;
+  base::ScopedObservation<views::Widget, views::WidgetObserver>
+      widget_observation_{this};
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_WINDRISE_GRID_BUTTON_H_
