diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h
new file mode 100644
index 0000000000..76ce7d0228
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h
@@ -0,0 +1,131 @@
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
+class ExpandOnHoverLock;
+class WindriseAiPanelView;
+
+// 竖向标签栏左下角「已连接 N 个 AI」卡片（B 版银色玻璃）。
+// 数据全部来自扩展：windrise.setConnectedAis 推连接列表，卡片只显示数量 /
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
+  // 标签栏收起到位时卡片的宽度（= 收起宽 - 两侧留白，和收起态标签同宽）。
+  // 收起态的「标志 + 数字」按这个宽度居中，而不是按这一刻的宽度：交叉淡的
+  // 时候卡片宽度还在变，按这一刻居中会让这一组一边淡入一边横移。
+  void SetCollapsedContentWidth(int width) { collapsed_content_width_ = width; }
+
+  // 面板点了底部按钮：转成 windrise.onAiPanelAction 发给扩展。
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
+  // views::AnimationDelegateViews（经 views::Button 继承；折叠 /
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
+  // ：折叠态 ⇄ 展开态内容真正的交叉淡。两组内容
+  // 同时在场、各自一层：旧的一组立刻按 T0 淡出 180ms；新的一组等卡片宽度走过
+  // 新旧宽度的中点（切换开始时已是目标宽度就立刻）再按 T0 淡入 180ms；无论如何
+  // 旧组透明度降到 0.5 时新组一定已经开始，两组之间不留空档。被打断：两组都从
+  // 当前透明度接着走。
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
+  // 面板开着时锁住竖向标签栏「悬停展开」的当前状态（同上游菜单 / 气泡的做法，
+  // ExpandOnHoverLockType::kKeepCurrentState）：否则收起态点卡片时悬停浮出会
+  // 在面板出现后再展开 / 先缩回再展开，挂在卡片上的面板跟着横移
+  // （补录 证据-动效-浏览器-04-补录/S3，见盘点 8.15）。
+  std::unique_ptr<ExpandOnHoverLock> expand_on_hover_lock_;
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
+  // 见 SetCollapsedContentWidth；0 = 不知道，退回按这一刻的宽度。
+  int collapsed_content_width_ = 0;
+  gfx::LinearAnimation fade_out_animation_;
+  gfx::LinearAnimation fade_in_animation_;
+  // AI 图标只在 AI 状态变化时解码一次（切换时不再解 PNG）。
+  std::vector<gfx::ImageSkia> decoded_icons_;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_TABS_VERTICAL_WINDRISE_AI_CARD_VIEW_H_
\ No newline at end of file
