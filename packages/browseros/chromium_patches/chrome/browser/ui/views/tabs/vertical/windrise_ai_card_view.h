diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h
new file mode 100644
index 0000000000..b38e2af5dd
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h
@@ -0,0 +1,118 @@
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
+#include "ui/gfx/animation/linear_animation.h"
+#include "ui/gfx/image/image_skia.h"
+#include "ui/views/animation/animation_delegate_views.h"
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
+// views::Button 本身已是 AnimationDelegateViews（ui/views/controls/button/button.h），
+// 这里不再重复继承（重复继承在 158 上编不过：this → AnimationDelegate 二义）。
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
+  // views::AnimationDelegateViews（经 views::Button 继承；motion-1115 N2：折叠 /
+  // 展开内容交叉淡。不是 fade_out_animation_ / fade_in_animation_ 的回调原样交给
+  // views::Button，保持 Button 自己悬停动画的行为）：
+  void AnimationProgressed(const gfx::Animation* animation) override;
+  void AnimationEnded(const gfx::Animation* animation) override;
+
+ protected:
+  // views::Button:
+  void StateChanged(ButtonState old_state) override;
+
+ private:
+  void OnAiStateChanged();
+  void RebuildFromState();
+  void ShowPanel();
+
+  // 动效-1115 N2（审查 P4 改）：折叠态 ⇄ 展开态内容真正的交叉淡。两组内容
+  // 同时在场、各自一层：旧的一组立刻按 T0 淡出 180ms；新的一组等卡片宽度走过
+  // 新旧宽度的中点再按 T0 淡入 180ms（还不知道目标宽度、或宽度不变时，旧组淡到
+  // 一半就开始淡入），两组之间不留空档。被打断：两组都从当前透明度接着走。
+  // 减少动态 / 不在窗口里：直接换。
+  // 组下标：0 = 展开态（文字 / 齿轮 / AI 标志），1 = 折叠态（风桥标志 + 数字）。
+  static size_t GroupIndex(bool collapsed_group) { return collapsed_group ? 1 : 0; }
+  std::vector<views::View*> GroupViews(bool collapsed_group) const;
+  bool GroupShown(bool collapsed_group) const;
+  void ApplyGroupOpacity(bool collapsed_group);
+  void ClearContentLayers();
+  void MaybeStartFadeIn(bool force);
+  bool FadeInGateReached() const;
+  void FinishFadeIfDone();
+  void LayoutExpandedGroup(int center_y);
+  void LayoutCollapsedGroup(int center_y);
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
+  // 目标状态（侧栏收没收）。
+  bool collapsed_ = false;
+  // 两组内容当前的透明度和这一段淡入 / 淡出的起点（下标见 GroupIndex）。
+  float group_opacity_[2] = {1.0f, 0.0f};
+  float group_from_[2] = {1.0f, 0.0f};
+  // 新的一组在等「宽度过中点」才开始淡入。
+  bool fade_in_pending_ = false;
+  // 这次切换开始时卡片的宽度；两种状态稳定时各自量到的宽度（0 = 还没量到）。
+  int fade_start_width_ = 0;
+  int settled_width_[2] = {0, 0};
+  gfx::LinearAnimation fade_out_animation_;
+  gfx::LinearAnimation fade_in_animation_;
+  // AI 图标只在 AI 状态变化时解码一次（动效-1115 N2：切换时不再解 PNG）。
+  std::vector<gfx::ImageSkia> decoded_icons_;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_CARD_VIEW_H_
\ No newline at end of file
