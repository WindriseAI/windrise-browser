diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h
new file mode 100644
index 0000000000..454ddf6ad2
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h
@@ -0,0 +1,38 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_PANEL_VIEW_H_
+#define CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_PANEL_VIEW_H_
+
+#include "base/callback_list.h"
+#include "base/memory/raw_ptr.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/views/bubble/bubble_dialog_delegate_view.h"
+
+class BrowserWindowInterface;
+
+// 「已连接 N 个 AI」卡片点开后浮出的面板（方案 N7，B 版银色玻璃）。
+// 行 = 一家 AI（名字 + 上次调用时间），底部一行按钮「打开风桥 / 闪办广场 / 设置」
+// 发给扩展。不放额度、剩余时长、公网地址。数据全部来自扩展推的状态。
+class WindriseAiPanelView : public views::BubbleDialogDelegateView {
+  METADATA_HEADER(WindriseAiPanelView, views::BubbleDialogDelegateView)
+ public:
+  explicit WindriseAiPanelView(BrowserWindowInterface* browser);
+  WindriseAiPanelView(const WindriseAiPanelView&) = delete;
+  WindriseAiPanelView& operator=(const WindriseAiPanelView&) = delete;
+  ~WindriseAiPanelView() override;
+
+  // 以 `anchor`（卡片）为锚点浮出：展开时在卡片上方，折叠时在卡片右边。
+  void ShowAtAnchor(views::View* anchor, bool collapsed);
+
+ private:
+  void RebuildContent();
+  void NotifyAction(const char* action);
+
+  raw_ptr<BrowserWindowInterface> browser_ = nullptr;
+  raw_ptr<views::View> rows_container_ = nullptr;
+  base::CallbackListSubscription state_subscription_;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_PANEL_VIEW_H_
\ No newline at end of file
