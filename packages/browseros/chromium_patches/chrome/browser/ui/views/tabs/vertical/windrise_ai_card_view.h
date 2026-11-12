diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h
new file mode 100644
index 0000000..880d04b
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h
@@ -0,0 +1,73 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_CARD_VIEW_H_
+#define CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_CARD_VIEW_H_
+
+#include <memory>
+#include <vector>
+
+#include "base/callback_list.h"
+#include "base/memory/raw_ptr.h"
+#include "base/scoped_observation.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/views/controls/button/button.h"
+#include "ui/views/controls/image_view.h"
+#include "ui/views/controls/label.h"
+#include "ui/views/view.h"
+#include "ui/views/widget/widget_observer.h"
+
+class BrowserWindowInterface;
+class WindriseAiPanelView;
+
+// 竖向标签栏左下角「已连接 N 个 AI」卡片（方案 N7，B 版银色玻璃）。
+// 数据全部来自扩展：browserOS.setConnectedAis 推连接列表，卡片只显示数量 /
+// AI 标志 / 齿轮；点卡片向右上方浮出 WindriseAiPanelView。收起（折叠标签栏）
+// 时只剩风桥标志 + 数字。
+class WindriseAiCardView : public views::Button,
+                            public views::WidgetObserver {
+  METADATA_HEADER(WindriseAiCardView, views::Button)
+ public:
+  explicit WindriseAiCardView(BrowserWindowInterface* browser);
+  WindriseAiCardView(const WindriseAiCardView&) = delete;
+  WindriseAiCardView& operator=(const WindriseAiCardView&) = delete;
+  ~WindriseAiCardView() override;
+
+  // 标签栏折叠 / 展开时由 VerticalTabStripRegionView 调。
+  void SetCollapsed(bool collapsed);
+
+  // 面板点了底部按钮：转成 browserOS.onAiPanelAction 发给扩展。
+  void NotifyPanelAction(const char* action);
+
+  // views::View:
+  gfx::Size CalculatePreferredSize(
+      const views::SizeBounds& available_size) const override;
+  void Layout(PassKey pass_key) override;
+  void OnPaintBackground(gfx::Canvas* canvas) override;
+  void OnThemeChanged() override;
+
+  // views::WidgetObserver:
+  void OnWidgetDestroyed(views::Widget* widget) override;
+
+ private:
+  void OnAiStateChanged();
+  void RebuildFromState();
+  void ShowPanel();
+
+  raw_ptr<BrowserWindowInterface> browser_ = nullptr;
+  raw_ptr<WindriseAiPanelView> panel_ = nullptr;
+
+  raw_ptr<views::ImageView> mark_ = nullptr;
+  raw_ptr<views::Label> count_label_ = nullptr;
+  raw_ptr<views::Label> text_ = nullptr;
+  raw_ptr<views::ImageView> gear_ = nullptr;
+  std::vector<raw_ptr<views::ImageView>> ai_icons_;
+
+  base::CallbackListSubscription state_subscription_;
+  base::ScopedObservation<views::Widget, views::WidgetObserver>
+      panel_observation_{this};
+  bool collapsed_ = false;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_CARD_VIEW_H_
\ No newline at end of file
