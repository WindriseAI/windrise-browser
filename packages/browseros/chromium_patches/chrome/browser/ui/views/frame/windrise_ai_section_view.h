diff --git a/chrome/browser/ui/views/frame/windrise_ai_section_view.h b/chrome/browser/ui/views/frame/windrise_ai_section_view.h
new file mode 100644
index 0000000000..f472ea7b68
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_ai_section_view.h
@@ -0,0 +1,212 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_AI_SECTION_VIEW_H_
+#define CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_AI_SECTION_VIEW_H_
+
+#include <map>
+#include <optional>
+#include <string>
+#include <vector>
+
+#include "base/callback_list.h"
+#include "base/memory/raw_ptr.h"
+#include "base/memory/weak_ptr.h"
+#include "base/scoped_multi_source_observation.h"
+#include "base/timer/timer.h"
+#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
+#include "components/tabs/public/tab_interface.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/gfx/animation/animation_delegate.h"
+#include "ui/gfx/animation/slide_animation.h"
+#include "ui/gfx/image/image_skia.h"
+#include "ui/views/view.h"
+#include "ui/views/view_observer.h"
+
+class BrowserWindowInterface;
+class PrefService;
+class TabStripModel;
+
+
+// 风桥 N6（用户 11-05 18:12 / 18:15，漏做核查 L2、#55 #56 #58）：竖向侧栏的
+// 下段——AI 的标签。
+//
+// - 人的标签在上段（上游 TabStripView，从上往下排、正常滚动）；AI 的标签在
+//   这一段，贴着底部、从下往上排，滚轮方向和上段相反。
+// - 每个 AI 一组「组合页」（组名 = AI 名，数据来自 browserOS.setAiTabs），默认
+//   收起成一行，点开才把这个 AI 的全部标签展开出来（往上展开，点的那一行不动）。
+// - 两段之间的分隔条：两段没碰到时不显示；放不下（碰到）时才淡入。用户可以
+//   上下拖动分配两段的高度，松手位置按比例记在偏好里。
+// - 收起态（70 宽）：组合页一行是「AI 标志 + 标签数」，标签一行是「图标 +
+//   一个字」，和上段标签、左下卡片同构。
+//
+// 放在 content_area_view_ 里「+」（bottom_button_container_，唯一一个可无限
+// 伸长的子视图）和左下卡片之间：「+」吃掉两段之间的空白，这一段就贴着卡片。
+// 段高由本视图按上段的需要和用户的比例算出来（CalculatePreferredSize），上段
+// （kScaleToMinimum）由 FlexLayout 把剩下的给它。
+//
+// 这一段里的标签不是上游 TabView，是对模型里那些标签的代理行：点一下切过去、
+// 中键或「×」关掉。上游标签的拖动、右键菜单、悬停卡、声音指示这一段没有。
+class WindriseAiSectionView : public views::View,
+                              public TabStripModelObserver,
+                              public views::ViewObserver,
+                              public gfx::AnimationDelegate {
+  METADATA_HEADER(WindriseAiSectionView, views::View)
+
+ public:
+  WindriseAiSectionView(BrowserWindowInterface* browser,
+                        TabStripModel* tab_strip_model,
+                        PrefService* prefs);
+  WindriseAiSectionView(const WindriseAiSectionView&) = delete;
+  WindriseAiSectionView& operator=(const WindriseAiSectionView&) = delete;
+  ~WindriseAiSectionView() override;
+
+  // 上段（人的标签条）和它下面的「+」。段高要看上段要多高、最少多高。
+  // 标签条拆掉时传 nullptr。
+  void SetHumanSection(views::View* tab_strip, views::View* new_tab_container);
+
+  // views::View:
+  gfx::Size CalculatePreferredSize(
+      const views::SizeBounds& available_size) const override;
+  void Layout(PassKey) override;
+  void OnPaint(gfx::Canvas* canvas) override;
+  bool OnMousePressed(const ui::MouseEvent& event) override;
+  bool OnMouseDragged(const ui::MouseEvent& event) override;
+  void OnMouseReleased(const ui::MouseEvent& event) override;
+  void OnMouseCaptureLost() override;
+  void OnMouseMoved(const ui::MouseEvent& event) override;
+  void OnMouseExited(const ui::MouseEvent& event) override;
+  bool OnMouseWheel(const ui::MouseWheelEvent& event) override;
+  void OnScrollEvent(ui::ScrollEvent* event) override;
+  void OnGestureEvent(ui::GestureEvent* event) override;
+  ui::Cursor GetCursor(const ui::MouseEvent& event) override;
+  void OnThemeChanged() override;
+
+  // TabStripModelObserver:
+  void OnTabStripModelChanged(TabStripModel* tab_strip_model,
+                              const TabStripModelChange& change,
+                              const TabStripSelectionChange& selection) override;
+  void OnTabChangedAt(tabs::TabInterface* tab,
+                      TabChangeType change_type) override;
+  void OnTabGroupChanged(const TabGroupChange& change) override;
+  void OnTabStripModelDestroyed(TabStripModel* tab_strip_model) override;
+
+  // views::ViewObserver:
+  void OnViewPreferredSizeChanged(views::View* observed_view) override;
+  void OnViewBoundsChanged(views::View* observed_view) override;
+  void OnViewIsDeleting(views::View* observed_view) override;
+
+  // gfx::AnimationDelegate:
+  void AnimationProgressed(const gfx::Animation* animation) override;
+  void AnimationEnded(const gfx::Animation* animation) override;
+
+ private:
+  class Row;
+  class ScrollIndicator;
+
+  // 一行要画的东西。key：组合页行是 AI 名，标签行是模型里的标签。
+  struct RowData {
+    bool is_header = false;
+    std::u16string ai_name;
+    // 标签行对应的标签（用句柄，标签关掉以后取出来是空，不留悬空指针）。
+    tabs::TabHandle tab;
+    // 组合页行：这个 AI 的标志（扩展经 setConnectedAis 推的 PNG）；没有就画
+    // 首字方块。
+    gfx::ImageSkia ai_icon;
+    int tab_count = 0;
+    bool expanded = false;
+    bool active = false;     // 标签行：当前标签；组合页行：收起着且里面有当前标签
+    bool operating = false;  // 组合页行：这个 AI 正在操作
+  };
+
+  // 段高怎么分（都不含本视图的上下外边距）。
+  struct Allocation {
+    int natural = 0;     // 这一段全放下要多高（分隔条那一条 + 内容）
+    int target = 0;      // 这一刻应该多高
+    int min_height = 0;  // 碰到时最少多高
+    int max_height = 0;  // 碰到时最多多高（上段要留出它的最小高）
+    int available = 0;   // 两段一共能用多高
+    bool colliding = false;
+  };
+
+  // 数据变了（AI 段、标签进出、展开收起）：重建行，段高按需要过渡。
+  void Rebuild(bool animate_height);
+  // 标签标题 / 图标变了：只更新行的内容。
+  void RefreshRowContents();
+  std::vector<RowData> CollectRows() const;
+
+  Allocation ComputeAllocation() const;
+  int ContentHeight() const;
+  int SplitterHeight() const;
+  int RowHeight() const;
+  int RowGap() const;
+  // 父视图里除了上段、本视图以外，其余子视图（含外边距）占掉的高。
+  int FixedSiblingsHeight() const;
+  int HumanPreferredHeight() const;
+  int HumanMinimumHeight() const;
+  double StoredRatio() const;
+
+  // 段高从眼前的高度过渡到新目标（可以随时被下一次打断，从当前值接着走）。
+  void StartHeightTransition();
+  void SetScrollOffset(int offset);
+  int MaxScrollOffset() const;
+  void ShowScrollIndicator();
+  void ToggleExpanded(const std::u16string& ai_name);
+  void ActivateTab(tabs::TabInterface* tab);
+  void CloseTab(tabs::TabInterface* tab);
+  bool IsRail() const;
+  bool IsInSplitter(const gfx::Point& point) const;
+  void UpdateSplitterHover(bool hovered);
+  void OnAiStateChanged();
+  void OnRowPressed(Row* row, const ui::Event& event);
+  // 行的点击动作放到下一个任务里做：展开 / 收起、关标签都会重建行，不能在
+  // 被点的那一行自己的回调里把它删掉。
+  void PerformRowAction(RowData data, bool close);
+  // 松手 / 被打断：按比例记下位置，从眼前的高走到目标。
+  void FinishDrag();
+  void UpdateSplitterVisibility();
+
+  raw_ptr<BrowserWindowInterface> browser_;
+  raw_ptr<TabStripModel> tab_strip_model_;
+  raw_ptr<PrefService> prefs_;
+  base::CallbackListSubscription ai_state_subscription_;
+
+  raw_ptr<views::View> tab_strip_ = nullptr;
+  raw_ptr<views::View> new_tab_container_ = nullptr;
+  base::ScopedMultiSourceObservation<views::View, views::ViewObserver>
+      observations_{this};
+
+  // 可视区（贴图层、按边界裁）；行和滚动细条都在它里面，细条永远排最后。
+  raw_ptr<views::View> viewport_ = nullptr;
+  raw_ptr<ScrollIndicator> indicator_ = nullptr;
+  std::vector<raw_ptr<Row>> rows_;
+  std::vector<RowData> row_data_;
+  // 每个 AI 的组合页展开了没有（只在本窗口、本次运行里记）。默认收起。
+  std::map<std::u16string, bool> expanded_;
+
+  // 内容藏在可视区下沿以下的高度：0 = 贴底（最早的在最下面）。
+  int scroll_offset_ = 0;
+
+  // 段高过渡（出现、消失、展开、收起、松手回弹）。
+  gfx::SlideAnimation height_animation_{this};
+  int height_from_ = 0;
+  // 拖分隔条时这一刻的段高（含边界处的橡皮筋）；没在拖就是空。
+  std::optional<int> drag_height_;
+  int drag_start_height_ = 0;
+  int drag_start_y_ = 0;
+  bool drag_started_ = false;
+  bool pressed_in_splitter_ = false;
+
+  // 分隔条淡入淡出；悬停 / 拖动时把手加深。
+  gfx::SlideAnimation splitter_animation_{this};
+  gfx::SlideAnimation grip_animation_{this};
+  bool splitter_hovered_ = false;
+
+  // 滚动时右侧那根细条：淡入 180ms、停 0.9 秒、淡出 400ms。
+  gfx::SlideAnimation indicator_animation_{this};
+  base::OneShotTimer indicator_hide_timer_;
+
+  base::WeakPtrFactory<WindriseAiSectionView> weak_factory_{this};
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_AI_SECTION_VIEW_H_
