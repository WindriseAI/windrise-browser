diff --git a/chrome/browser/ui/color/windrise_color_mixer.h b/chrome/browser/ui/color/windrise_color_mixer.h
new file mode 100644
index 0000000000..fb10a2a7ac
--- /dev/null
+++ b/chrome/browser/ui/color/windrise_color_mixer.h
@@ -0,0 +1,20 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_COLOR_WINDRISE_COLOR_MIXER_H_
+#define CHROME_BROWSER_UI_COLOR_WINDRISE_COLOR_MIXER_H_
+
+#include "ui/color/color_provider_key.h"
+
+namespace ui {
+class ColorProvider;
+}  // namespace ui
+
+// 风桥整窗配色。最后运行，覆盖 frame / 工具栏 / 地址栏 / 标签 / 弹层的颜色。
+// 取值来自 browseros::windrise 的外观快照（profile 偏好
+// browseros.windrise.theme.*，见 browseros/core/windrise_theme.h），所以改偏好
+// 即可在运行时换色，不用重编。
+void AddWindriseColorMixer(ui::ColorProvider* provider,
+                           const ui::ColorProviderKey& key);
+
+#endif  // CHROME_BROWSER_UI_COLOR_WINDRISE_COLOR_MIXER_H_
