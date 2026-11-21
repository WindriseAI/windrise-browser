diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h
new file mode 100644
index 0000000000..f4faf00bc1
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h
@@ -0,0 +1,49 @@
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
+#include "ui/views/layout/box_layout_view.h"
+
+class BrowserWindowInterface;
+
+namespace gfx {
+class Canvas;
+}  // namespace gfx
+
+// 「已连接 N 个 AI」卡片点开后浮出的面板（B 版银色玻璃；设计稿
+// ntp-zh-panel / ntp-en-panel）。
+// 从上到下：标题「已连接 N 个 AI」、每家 AI 一行（名字 + 上次调用时间）、
+// 「接一个 AI」白底宽按钮、.5px 线、底部文字行「打开风桥 / 闪办广场 / 设置」。
+// 按钮发给扩展（windrise.onAiPanelAction）。不放额度、剩余时长、公网地址。
+// 数据全部来自扩展推的状态。
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
+  // views::View:
+  void OnPaintBackground(gfx::Canvas* canvas) override;
+
+ private:
+  void RebuildContent();
+  void NotifyAction(const char* action);
+
+  raw_ptr<BrowserWindowInterface> browser_ = nullptr;
+  raw_ptr<views::BoxLayoutView> rows_container_ = nullptr;
+  base::CallbackListSubscription state_subscription_;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_PANEL_VIEW_H_
\ No newline at end of file
