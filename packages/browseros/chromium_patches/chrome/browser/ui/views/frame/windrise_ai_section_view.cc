diff --git a/chrome/browser/ui/views/frame/windrise_ai_section_view.cc b/chrome/browser/ui/views/frame/windrise_ai_section_view.cc
new file mode 100644
index 0000000000..489de31b62
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_ai_section_view.cc
@@ -0,0 +1,1205 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/frame/windrise_ai_section_view.h"
+
+#include <algorithm>
+#include <cmath>
+#include <cstdlib>
+#include <utility>
+
+#include "base/containers/span.h"
+#include "base/functional/bind.h"
+#include "base/numerics/safe_conversions.h"
+#include "base/strings/string_number_conversions.h"
+#include "base/task/sequenced_task_runner.h"
+#include "base/time/time.h"
+#include "cc/paint/paint_flags.h"
+#include "cc/paint/paint_shader.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/browseros/core/windrise_rail_glyph.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
+#include "chrome/browser/ui/layout_constants.h"
+#include "chrome/browser/ui/tab_ui_helper.h"
+#include "chrome/browser/ui/tabs/tab_enums.h"
+#include "chrome/browser/ui/tabs/tab_group_model.h"
+#include "chrome/browser/ui/tabs/tab_strip_model.h"
+#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
+#include "chrome/browser/ui/tabs/vertical_tab_strip_state.h"
+#include "chrome/grit/generated_resources.h"
+#include "components/prefs/pref_service.h"
+#include "components/tabs/public/tab_group.h"
+#include "components/vector_icons/vector_icons.h"
+#include "third_party/skia/include/core/SkColor.h"
+#include "third_party/skia/include/core/SkPoint.h"
+#include "third_party/skia/include/core/SkTileMode.h"
+#include "url/gurl.h"
+#include "ui/base/cursor/cursor.h"
+#include "ui/base/l10n/l10n_util.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/base/models/image_model.h"
+#include "ui/color/color_provider.h"
+#include "ui/compositor/layer.h"
+#include "ui/events/event.h"
+#include "ui/gfx/animation/animation.h"
+#include "ui/gfx/animation/tween.h"
+#include "ui/gfx/canvas.h"
+#include "ui/gfx/font.h"
+#include "ui/gfx/font_list.h"
+#include "ui/gfx/geometry/point_conversions.h"
+#include "ui/gfx/geometry/rect.h"
+#include "ui/gfx/geometry/rect_f.h"
+#include "ui/gfx/image/image.h"
+#include "ui/gfx/paint_vector_icon.h"
+#include "ui/gfx/scoped_canvas.h"
+#include "ui/gfx/text_utils.h"
+#include "ui/base/cursor/mojom/cursor_type.mojom-shared.h"
+#include "ui/views/accessibility/view_accessibility.h"
+#include "ui/views/controls/button/button.h"
+#include "ui/views/view_class_properties.h"
+
+namespace {
+
+using browseros::windrise::GetLayoutMetrics;
+using browseros::windrise::LayoutMetric;
+
+constexpr int kIconSize = 16;
+// 展开态标题从 x = 内左边距 10 + 图标 16 + 8 = 34 起，和上段标签同一条竖线。
+constexpr int kIconTitleGap = 8;
+// 标签上下间距（theme-v2 §3「上下间距 2」）。
+constexpr int kRowGap = 2;
+// 收起态那一个字 / 数字的格子宽，和上段标签、左下卡片同一个格子。
+constexpr int kRailGlyphCell = 14;
+// 「×」的点击区（图标 16，四周各让 4）。
+constexpr int kCloseHitSize = 24;
+// 拖分隔条：先动 3 DIP 才算开始拖（鼠标按下时手抖不改高度）。
+constexpr int kDragThreshold = 3;
+// 边界处的橡皮筋系数（apple-design §9）。
+constexpr float kRubberBandConstant = 0.55f;
+// 默认两段的分法：碰到时下段占两段可用高的 40%。
+constexpr double kDefaultRatio = 0.4;
+constexpr double kMinRatio = 0.1;
+constexpr double kMaxRatio = 0.9;
+// 滚轮方向。用户 11-05 18:12 先说过 AI 段「反过来滚滚轮」（R186），18:15 改口
+// 「算了，AI区不单独滚动了，就正常滚动。」（R189，产品需求清单记 R186 已被它
+// 取代）。所以这里照 R189：和上段同一个方向（+1）。协调人如果裁定要反向，
+// 只改这一个数为 -1。
+constexpr int kWheelDirection = 1;
+
+// 动效（review-animations 十条见交付稿第五节）。曲线都是 WINDRISE_EASE_OUT
+// （.23, 1, .32, 1）；减少动态效果时 RichAnimationDuration 给 0，直接到位。
+constexpr base::TimeDelta kHeightDuration = base::Milliseconds(200);
+constexpr base::TimeDelta kSplitterFadeDuration = base::Milliseconds(150);
+constexpr base::TimeDelta kGripDuration = base::Milliseconds(150);
+constexpr base::TimeDelta kIndicatorInDuration = base::Milliseconds(180);
+constexpr base::TimeDelta kIndicatorOutDuration = base::Milliseconds(400);
+constexpr base::TimeDelta kIndicatorHold = base::Milliseconds(900);
+
+// 分隔条把手、滚动细条：墨色 28%，悬停 / 拖动 45%（和桌面端细条同一套）。
+constexpr float kGripRestAlpha = 0.28f;
+constexpr float kGripActiveAlpha = 0.45f;
+
+int Metric(LayoutMetric metric) {
+  return GetLayoutMetrics().Get(metric);
+}
+
+int RubberBand(int overshoot, int dimension) {
+  if (overshoot <= 0 || dimension <= 0) {
+    return 0;
+  }
+  const float x = static_cast<float>(overshoot);
+  const float d = static_cast<float>(dimension);
+  return base::ClampRound(x * d * kRubberBandConstant /
+                          (d + kRubberBandConstant * x));
+}
+
+SkColor WithAlpha(SkColor color, float alpha) {
+  return SkColorSetA(color, base::ClampRound(SkColorGetA(color) *
+                                             std::clamp(alpha, 0.0f, 1.0f)));
+}
+
+// 按字形把文字放到某条中线上（甲45：不按行框居中）：基线 = 中线 + 半个
+// 大写字母高。文字框高等于字体高时 RenderText 不再挪基线。
+void DrawTextOnCenterLine(gfx::Canvas* canvas,
+                          std::u16string_view text,
+                          const gfx::FontList& font_list,
+                          SkColor color,
+                          int x,
+                          int width,
+                          int center_y,
+                          int flags) {
+  if (width <= 0 || text.empty()) {
+    return;
+  }
+  const int baseline = center_y + (font_list.GetCapHeight() + 1) / 2;
+  const gfx::Rect rect(x, baseline - font_list.GetBaseline(), width,
+                       font_list.GetHeight());
+  canvas->DrawStringRectWithFlags(text, font_list, color, rect,
+                                  flags | gfx::Canvas::NO_SUBPIXEL_RENDERING);
+}
+
+gfx::FontList DefaultFontWithSize(int size, gfx::Font::Weight weight) {
+  const gfx::FontList base_font;
+  return base_font.Derive(size - base_font.GetFontSize(), gfx::Font::NORMAL,
+                          weight);
+}
+
+}  // namespace
+
+// ---------------------------------------------------------------------------
+// 一行：组合页行（一个 AI）或标签行。都是可点、可聚焦的按钮。
+
+class WindriseAiSectionView::Row : public views::Button {
+  METADATA_HEADER(Row, views::Button)
+
+ public:
+  explicit Row(WindriseAiSectionView* section) : section_(section) {
+    SetCallback(base::BindRepeating(
+        [](Row* row, const ui::Event& event) {
+          row->section_->OnRowPressed(row, event);
+        },
+        base::Unretained(this)));
+    SetTriggerableEventFlags(ui::EF_LEFT_MOUSE_BUTTON |
+                             ui::EF_MIDDLE_MOUSE_BUTTON);
+  }
+  Row(const Row&) = delete;
+  Row& operator=(const Row&) = delete;
+  ~Row() override = default;
+
+  void SetData(const RowData& data) {
+    data_ = data;
+    std::u16string name = data_.is_header ? data_.ai_name : TabTitle();
+    if (data_.is_header && data_.operating) {
+      name = l10n_util::GetStringFUTF16(IDS_WINDRISE_AI_OPERATING,
+                                        data_.ai_name);
+    }
+    SetTooltipText(name);
+    GetViewAccessibility().SetName(name);
+    SchedulePaint();
+  }
+  const RowData& data() const { return data_; }
+
+  // 「×」只在展开态的标签行、悬停或当前标签时出现。
+  gfx::Rect GetCloseHitRect() const {
+    const int inset_right = Metric(LayoutMetric::kTabContentsInsetRight);
+    const int icon_x = width() - inset_right - kIconSize;
+    return gfx::Rect(icon_x - (kCloseHitSize - kIconSize) / 2,
+                     (height() - kCloseHitSize) / 2, kCloseHitSize,
+                     kCloseHitSize);
+  }
+  bool ShowsClose() const {
+    return !data_.is_header && !section_->IsRail() &&
+           (data_.active || IsHoveredOrPressed());
+  }
+
+ protected:
+  void PaintButtonContents(gfx::Canvas* canvas) override {
+    const ui::ColorProvider* colors = GetColorProvider();
+    if (!colors) {
+      return;
+    }
+    const bool rail = section_->IsRail();
+    const int radius = Metric(LayoutMetric::kTabRadius);
+    const SkColor active_bg =
+        colors->GetColor(kColorWindriseTabActiveBackground);
+    const SkColor text = colors->GetColor(kColorTabForegroundActiveFrameActive);
+    const SkColor text_secondary =
+        colors->GetColor(kColorTabForegroundInactiveFrameActive);
+
+    // 底：当前标签（或收起着、里面有当前标签的组合页）用当前标签底色；悬停
+    // 用它的一半，和上段标签的悬停同一个方向。
+    if (data_.active || IsHoveredOrPressed()) {
+      cc::PaintFlags flags;
+      flags.setAntiAlias(true);
+      flags.setColor(data_.active ? active_bg : WithAlpha(active_bg, 0.5f));
+      canvas->DrawRoundRect(gfx::RectF(GetLocalBounds()), radius, flags);
+    }
+
+    const int center_y = height() / 2;
+    const int inset_left = Metric(LayoutMetric::kTabContentsInsetLeft);
+    const int inset_right = Metric(LayoutMetric::kTabContentsInsetRight);
+    const gfx::FontList title_font;
+    const gfx::FontList glyph_font = DefaultFontWithSize(
+        Metric(LayoutMetric::kRailGlyphSize), title_font.GetFontWeight());
+
+    if (rail) {
+      // 收起态：图标 16 + 间距 5 + 一个字 / 数字的格子 14，一组在行里居中，
+      // 奇数余量给左边（和上段标签、左下卡片同一个算法，图标都在 x=10）。
+      const int gap = Metric(LayoutMetric::kRailGlyphGap);
+      const std::u16string glyph =
+          data_.is_header ? base::NumberToString16(data_.tab_count)
+                          : RailGlyph();
+      const int cell = std::max(
+          kRailGlyphCell, gfx::GetStringWidth(glyph, glyph_font));
+      const int group = kIconSize + gap + cell;
+      const int x = std::max(0, (width() - group + 1) / 2);
+      PaintIcon(canvas, x, center_y);
+      DrawTextOnCenterLine(canvas, glyph, glyph_font,
+                           data_.active || data_.is_header ? text
+                                                           : text_secondary,
+                           x + kIconSize + gap, cell, center_y,
+                           gfx::Canvas::TEXT_ALIGN_CENTER |
+                               gfx::Canvas::NO_ELLIPSIS);
+      return;
+    }
+
+    PaintIcon(canvas, inset_left, center_y);
+    const int text_x = inset_left + kIconSize + kIconTitleGap;
+    int text_right = width() - inset_right;
+
+    if (data_.is_header) {
+      // 右边：标签数 + 箭头。收起着箭头朝上（往上展开），展开了朝下。
+      const SkColor icon_color = text_secondary;
+      const gfx::ImageSkia chevron = gfx::CreateVectorIcon(
+          data_.expanded ? vector_icons::kKeyboardArrowDownIcon
+                         : vector_icons::kKeyboardArrowUpIcon,
+          kIconSize, icon_color);
+      text_right -= kIconSize;
+      canvas->DrawImageInt(chevron, text_right, center_y - kIconSize / 2);
+      const std::u16string count = base::NumberToString16(data_.tab_count);
+      const int count_width = gfx::GetStringWidth(count, title_font);
+      text_right -= 4 + count_width;
+      DrawTextOnCenterLine(canvas, count, title_font, text_secondary,
+                           text_right, count_width, center_y,
+                           gfx::Canvas::TEXT_ALIGN_LEFT |
+                               gfx::Canvas::NO_ELLIPSIS);
+      text_right -= kIconTitleGap;
+
+      // 名字用正文色、中等字重；正在操作时整句是「<名字> 正在操作」，名字
+      // 以外的部分用段标题色（丙01）。放不下整句时只放名字。
+      const gfx::FontList name_font =
+          title_font.DeriveWithWeight(gfx::Font::Weight::MEDIUM);
+      const SkColor status_color =
+          colors->GetColor(kColorWindriseSectionHeaderText);
+      const int available = text_right - text_x;
+      if (!data_.operating) {
+        DrawTextOnCenterLine(canvas, data_.ai_name, name_font, text, text_x,
+                             available, center_y,
+                             gfx::Canvas::TEXT_ALIGN_LEFT);
+        return;
+      }
+      size_t offset = 0;
+      const std::u16string full = l10n_util::GetStringFUTF16(
+          IDS_WINDRISE_AI_OPERATING, data_.ai_name, &offset);
+      offset = std::min(offset, full.size());
+      const std::u16string before = full.substr(0, offset);
+      const std::u16string after =
+          full.substr(std::min(full.size(), offset + data_.ai_name.size()));
+      const int before_width = gfx::GetStringWidth(before, title_font);
+      const int name_width = gfx::GetStringWidth(data_.ai_name, name_font);
+      const int after_width = gfx::GetStringWidth(after, title_font);
+      if (before_width + name_width + after_width > available) {
+        DrawTextOnCenterLine(canvas, data_.ai_name, name_font, text, text_x,
+                             available, center_y,
+                             gfx::Canvas::TEXT_ALIGN_LEFT);
+        return;
+      }
+      int x = text_x;
+      DrawTextOnCenterLine(canvas, before, title_font, status_color, x,
+                           before_width, center_y,
+                           gfx::Canvas::TEXT_ALIGN_LEFT |
+                               gfx::Canvas::NO_ELLIPSIS);
+      x += before_width;
+      DrawTextOnCenterLine(canvas, data_.ai_name, name_font, text, x,
+                           name_width, center_y,
+                           gfx::Canvas::TEXT_ALIGN_LEFT |
+                               gfx::Canvas::NO_ELLIPSIS);
+      x += name_width;
+      DrawTextOnCenterLine(canvas, after, title_font, status_color, x,
+                           after_width, center_y,
+                           gfx::Canvas::TEXT_ALIGN_LEFT |
+                               gfx::Canvas::NO_ELLIPSIS);
+      return;
+    }
+
+    // 标签行：图标 + 标题（+ 悬停 / 当前时的「×」）。
+    if (ShowsClose()) {
+      const gfx::ImageSkia close = gfx::CreateVectorIcon(
+          vector_icons::kCloseSmallIcon, kIconSize, text_secondary);
+      text_right = width() - inset_right - kIconSize;
+      canvas->DrawImageInt(close, text_right, center_y - kIconSize / 2);
+      text_right -= kIconTitleGap;
+    }
+    DrawTextOnCenterLine(canvas, TabTitle(), title_font,
+                         data_.active ? text : text_secondary, text_x,
+                         text_right - text_x, center_y,
+                         gfx::Canvas::TEXT_ALIGN_LEFT);
+  }
+
+  void StateChanged(ButtonState old_state) override {
+    views::Button::StateChanged(old_state);
+    SchedulePaint();
+  }
+
+ private:
+  bool IsHoveredOrPressed() const {
+    return GetState() == STATE_HOVERED || GetState() == STATE_PRESSED;
+  }
+
+  tabs::TabInterface* Tab() const { return data_.tab.Get(); }
+
+  std::u16string TabTitle() const {
+    tabs::TabInterface* tab = Tab();
+    const TabUIHelper* helper = tab ? TabUIHelper::From(tab) : nullptr;
+    return helper ? helper->GetTitle() : std::u16string();
+  }
+
+  std::u16string RailGlyph() const {
+    tabs::TabInterface* tab = Tab();
+    TabUIHelper* helper = tab ? TabUIHelper::From(tab) : nullptr;
+    if (!helper) {
+      return std::u16string();
+    }
+    const GURL url = helper->GetVisibleURL();
+    return browseros::windrise::RailGlyphFor(helper->GetTitle(), url.host());
+  }
+
+  // 组合页行画 AI 的标志（没有就画首字方块）；标签行画网站图标（没有就画
+  // 地球）。
+  void PaintIcon(gfx::Canvas* canvas, int x, int center_y) {
+    const int y = center_y - kIconSize / 2;
+    const ui::ColorProvider* colors = GetColorProvider();
+    const SkColor icon_color =
+        colors->GetColor(kColorTabForegroundInactiveFrameActive);
+    if (data_.is_header) {
+      if (!data_.ai_icon.isNull()) {
+        canvas->DrawImageInt(data_.ai_icon, 0, 0, data_.ai_icon.width(),
+                             data_.ai_icon.height(), x, y, kIconSize,
+                             kIconSize, true);
+        return;
+      }
+      cc::PaintFlags flags;
+      flags.setAntiAlias(true);
+      flags.setColor(colors->GetColor(kColorWindriseTileBackground));
+      canvas->DrawRoundRect(gfx::RectF(x, y, kIconSize, kIconSize), 4, flags);
+      const gfx::FontList initial_font = DefaultFontWithSize(
+          10, gfx::Font::Weight::SEMIBOLD);
+      DrawTextOnCenterLine(
+          canvas, browseros::windrise::RailGlyphForTitle(data_.ai_name),
+          initial_font,
+          colors->GetColor(kColorTabForegroundActiveFrameActive), x, kIconSize,
+          center_y,
+          gfx::Canvas::TEXT_ALIGN_CENTER | gfx::Canvas::NO_ELLIPSIS);
+      return;
+    }
+    tabs::TabInterface* tab = Tab();
+    TabUIHelper* helper = tab ? TabUIHelper::From(tab) : nullptr;
+    ui::ImageModel favicon = helper ? helper->GetFavicon() : ui::ImageModel();
+    gfx::ImageSkia image;
+    if (!favicon.IsEmpty()) {
+      image = favicon.Rasterize(colors);
+    }
+    if (image.isNull()) {
+      image = gfx::CreateVectorIcon(vector_icons::kGlobeIcon, kIconSize,
+                                    icon_color);
+    }
+    canvas->DrawImageInt(image, 0, 0, image.width(), image.height(), x, y,
+                         kIconSize, kIconSize, true);
+  }
+
+  const raw_ptr<WindriseAiSectionView> section_;
+  RowData data_;
+};
+
+BEGIN_METADATA(WindriseAiSectionView, Row)
+END_METADATA
+
+// ---------------------------------------------------------------------------
+// 可视区右侧的滚动细条：只在内容放不下、刚滚过的时候淡入，停 0.9 秒后淡出。
+
+class WindriseAiSectionView::ScrollIndicator : public views::View {
+  METADATA_HEADER(ScrollIndicator, views::View)
+
+ public:
+  explicit ScrollIndicator(WindriseAiSectionView* section)
+      : section_(section) {
+    SetCanProcessEventsWithinSubtree(false);
+    GetViewAccessibility().SetIsIgnored(true);
+  }
+  ScrollIndicator(const ScrollIndicator&) = delete;
+  ScrollIndicator& operator=(const ScrollIndicator&) = delete;
+  ~ScrollIndicator() override = default;
+
+  void OnPaint(gfx::Canvas* canvas) override {
+    const float opacity =
+        static_cast<float>(section_->indicator_animation_.GetCurrentValue());
+    const int max_offset = section_->MaxScrollOffset();
+    if (opacity <= 0.0f || max_offset <= 0 || !GetColorProvider()) {
+      return;
+    }
+    const int content = section_->ContentHeight();
+    const int track = height();
+    const int thumb =
+        std::min(track, std::max(16, track * track / std::max(1, content)));
+    // 偏移 0 = 贴底：细条也在最下面。
+    const int travel = track - thumb;
+    const int y = travel - base::ClampRound(static_cast<float>(travel) *
+                                            section_->scroll_offset_ /
+                                            max_offset);
+    cc::PaintFlags flags;
+    flags.setAntiAlias(true);
+    flags.setColor(WithAlpha(
+        GetColorProvider()->GetColor(kColorTabForegroundActiveFrameActive),
+        kGripRestAlpha * opacity));
+    canvas->DrawRoundRect(gfx::RectF(0, y, width(), thumb), width() / 2.0f,
+                          flags);
+  }
+
+ private:
+  const raw_ptr<WindriseAiSectionView> section_;
+};
+
+BEGIN_METADATA(WindriseAiSectionView, ScrollIndicator)
+END_METADATA
+
+// ---------------------------------------------------------------------------
+
+WindriseAiSectionView::WindriseAiSectionView(BrowserWindowInterface* browser,
+                                             TabStripModel* tab_strip_model,
+                                             PrefService* prefs)
+    : browser_(browser), tab_strip_model_(tab_strip_model), prefs_(prefs) {
+  viewport_ = AddChildView(std::make_unique<views::View>());
+  viewport_->SetPaintToLayer();
+  viewport_->layer()->SetFillsBoundsOpaquely(false);
+  viewport_->layer()->SetMasksToBounds(true);
+  indicator_ =
+      viewport_->AddChildView(std::make_unique<ScrollIndicator>(this));
+
+  height_animation_.SetSlideDuration(
+      gfx::Animation::RichAnimationDuration(kHeightDuration));
+  height_animation_.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
+  splitter_animation_.SetSlideDuration(
+      gfx::Animation::RichAnimationDuration(kSplitterFadeDuration));
+  splitter_animation_.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
+  grip_animation_.SetSlideDuration(
+      gfx::Animation::RichAnimationDuration(kGripDuration));
+  grip_animation_.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
+  indicator_animation_.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
+
+  if (tab_strip_model_) {
+    tab_strip_model_->AddObserver(this);
+  }
+  if (browseros::windrise::AiWindowState* ai_state =
+          browseros::windrise::AiWindowState::Attach(browser_)) {
+    ai_state_subscription_ = ai_state->AddObserver(base::BindRepeating(
+        &WindriseAiSectionView::OnAiStateChanged, base::Unretained(this)));
+  }
+  Rebuild(/*animate_height=*/false);
+}
+
+WindriseAiSectionView::~WindriseAiSectionView() {
+  if (tab_strip_model_) {
+    tab_strip_model_->RemoveObserver(this);
+  }
+  ai_state_subscription_ = {};
+  browseros::windrise::AiWindowState::Detach(browser_);
+}
+
+void WindriseAiSectionView::SetHumanSection(views::View* tab_strip,
+                                            views::View* new_tab_container) {
+  for (views::View* old : {tab_strip_.get(), new_tab_container_.get()}) {
+    if (old && observations_.IsObservingSource(old)) {
+      observations_.RemoveObservation(old);
+    }
+  }
+  tab_strip_ = tab_strip;
+  new_tab_container_ = new_tab_container;
+  for (views::View* view :
+       {tab_strip_.get(), new_tab_container_.get(), parent()}) {
+    if (view && !observations_.IsObservingSource(view)) {
+      observations_.AddObservation(view);
+    }
+  }
+  PreferredSizeChanged();
+}
+
+// --- 段高 ------------------------------------------------------------------
+
+int WindriseAiSectionView::SplitterHeight() const {
+  return Metric(LayoutMetric::kAiSplitterHeight);
+}
+
+int WindriseAiSectionView::RowHeight() const {
+  return Metric(LayoutMetric::kTabHeight);
+}
+
+int WindriseAiSectionView::RowGap() const {
+  return kRowGap;
+}
+
+int WindriseAiSectionView::ContentHeight() const {
+  const int count = static_cast<int>(row_data_.size());
+  return count == 0 ? 0 : count * RowHeight() + (count - 1) * RowGap();
+}
+
+int WindriseAiSectionView::FixedSiblingsHeight() const {
+  // 照 FlexLayout（content_area_view_ 开了 SetCollapseMargins）的算法：相邻
+  // 两个子视图之间的外边距取较大的一个。上段和本视图的高不算在内（它们分剩下
+  // 的），但它们的外边距算。
+  int total = 0;
+  int previous_bottom = 0;
+  bool first = true;
+  for (const views::View* child : parent()->children()) {
+    if (!child->GetVisible() ||
+        child->GetProperty(views::kViewIgnoredByLayoutKey)) {
+      continue;
+    }
+    const gfx::Insets* margins = child->GetProperty(views::kMarginsKey);
+    const int top = margins ? margins->top() : 0;
+    const int bottom = margins ? margins->bottom() : 0;
+    total += first ? top : std::max(previous_bottom, top);
+    first = false;
+    previous_bottom = bottom;
+    if (child != this && child != tab_strip_) {
+      total += child->GetPreferredSize().height();
+    }
+  }
+  return total + previous_bottom;
+}
+
+int WindriseAiSectionView::HumanPreferredHeight() const {
+  return tab_strip_ ? tab_strip_->GetPreferredSize().height() : 0;
+}
+
+int WindriseAiSectionView::HumanMinimumHeight() const {
+  return tab_strip_ ? tab_strip_->GetMinimumSize().height() : 0;
+}
+
+double WindriseAiSectionView::StoredRatio() const {
+  if (!prefs_ ||
+      !prefs_->FindPreference(browseros::windrise::kAiSectionRatioPref)) {
+    return kDefaultRatio;
+  }
+  return std::clamp(
+      prefs_->GetDouble(browseros::windrise::kAiSectionRatioPref), kMinRatio,
+      kMaxRatio);
+}
+
+WindriseAiSectionView::Allocation WindriseAiSectionView::ComputeAllocation()
+    const {
+  Allocation allocation;
+  if (row_data_.empty()) {
+    return allocation;
+  }
+  const int content = ContentHeight();
+  const int splitter = SplitterHeight();
+  // 分隔条那一条一直留着（没碰到时是透明的空白），所以分隔条出现、消失都
+  // 不改段高，也就不会反过来改变「碰没碰到」。
+  allocation.natural = splitter + content;
+  allocation.available =
+      parent() ? std::max(0, parent()->height() - FixedSiblingsHeight())
+               : allocation.natural;
+  allocation.colliding =
+      tab_strip_ &&
+      HumanPreferredHeight() + allocation.natural > allocation.available;
+  const int row = RowHeight();
+  allocation.min_height =
+      std::min(allocation.natural, splitter + std::min(content, row + row / 2));
+  allocation.max_height = std::max(
+      allocation.min_height,
+      std::min(allocation.natural,
+               allocation.available - HumanMinimumHeight()));
+  if (!allocation.colliding) {
+    allocation.target = allocation.natural;
+    return allocation;
+  }
+  // 碰到时：用户拖出来的比例是「AI 段最多占多高」（用户 11-05 18:19「分割条
+  // 拖动就是设定 AI 区最多占多高」）。但上段用不满它那一份时，多出来的给下段，
+  // 不在「+」下面留一块空白：下限是「两段可用高 − 上段要的高」。
+  const int fill_floor =
+      std::max(allocation.min_height,
+               allocation.available - HumanPreferredHeight());
+  allocation.min_height = std::min(fill_floor, allocation.max_height);
+  const int cap =
+      splitter + base::ClampRound(StoredRatio() *
+                                  std::max(0, allocation.available - splitter));
+  allocation.target =
+      std::clamp(cap, allocation.min_height, allocation.max_height);
+  return allocation;
+}
+
+gfx::Size WindriseAiSectionView::CalculatePreferredSize(
+    const views::SizeBounds& available_size) const {
+  if (drag_height_) {
+    return gfx::Size(0, std::max(0, *drag_height_));
+  }
+  const int target = ComputeAllocation().target;
+  if (height_animation_.is_animating()) {
+    // 从过渡开始时眼前的高走向「这一刻的」目标：过渡途中窗口变高、内容又变，
+    // 目标跟着变，不会走到一半跳一下。
+    return gfx::Size(0, gfx::Tween::IntValueBetween(
+                            height_animation_.GetCurrentValue(),
+                            height_from_, target));
+  }
+  return gfx::Size(0, target);
+}
+
+void WindriseAiSectionView::StartHeightTransition() {
+  if (!GetWidget() || !IsDrawn()) {
+    PreferredSizeChanged();
+    return;
+  }
+  // 从眼前的高接着走（可打断：上一次过渡没走完也从当前值出发）。
+  height_from_ = height();
+  height_animation_.Reset(0.0);
+  height_animation_.Show();
+  PreferredSizeChanged();
+}
+
+// --- 布局与绘制 -------------------------------------------------------------
+
+bool WindriseAiSectionView::IsRail() const {
+  // 收起到位（70 宽，减两边留白 = 54）时才换成「图标 + 一个字」，和上段标签
+  // 同一个时刻换。
+  return width() <=
+         tabs::kVerticalTabStripCollapsedWidth -
+             2 * GetLayoutConstant(
+                     LayoutConstant::kVerticalTabStripHorizontalPadding);
+}
+
+int WindriseAiSectionView::MaxScrollOffset() const {
+  return std::max(0, ContentHeight() - viewport_->height());
+}
+
+void WindriseAiSectionView::Layout(PassKey) {
+  const int splitter = row_data_.empty() ? 0 : SplitterHeight();
+  viewport_->SetBounds(0, splitter, width(), std::max(0, height() - splitter));
+  scroll_offset_ = std::clamp(scroll_offset_, 0, MaxScrollOffset());
+
+  // 从下往上排：第 0 行贴着可视区下沿；scroll_offset_ 是藏在下沿以下的高度。
+  const int row_height = RowHeight();
+  int from_bottom = 0;
+  for (Row* row : rows_) {
+    const int y =
+        viewport_->height() + scroll_offset_ - from_bottom - row_height;
+    row->SetBounds(0, y, width(), row_height);
+    from_bottom += row_height + RowGap();
+  }
+  constexpr int kIndicatorWidth = 4;
+  indicator_->SetBounds(std::max(0, width() - kIndicatorWidth - 2), 0,
+                        kIndicatorWidth, viewport_->height());
+  UpdateSplitterVisibility();
+}
+
+void WindriseAiSectionView::UpdateSplitterVisibility() {
+  const bool colliding = ComputeAllocation().colliding;
+  if (colliding && !splitter_animation_.IsShowing()) {
+    splitter_animation_.Show();
+  } else if (!colliding && splitter_animation_.IsShowing()) {
+    splitter_animation_.Hide();
+  }
+  SchedulePaint();
+}
+
+void WindriseAiSectionView::OnPaint(gfx::Canvas* canvas) {
+  views::View::OnPaint(canvas);
+  const float splitter_opacity =
+      static_cast<float>(splitter_animation_.GetCurrentValue());
+  if (row_data_.empty() || splitter_opacity <= 0.0f || !GetColorProvider()) {
+    return;
+  }
+  // 分隔条：一根物理 1px 的细线，两头 18% 的长度渐隐（不是一条死线）；正中
+  // 一个把手，平时墨色 28%，悬停 / 拖动加深到 45%，提示可以拖。
+  const float center_y = SplitterHeight() / 2.0f;
+  const SkColor line = GetColorProvider()->GetColor(kColorWindriseSectionDivider);
+  {
+    gfx::ScopedCanvas scoped(canvas);
+    const float scale = canvas->UndoDeviceScaleFactor();
+    const float left = 0.0f;
+    const float right = width() * scale;
+    const float y = std::floor(center_y * scale);
+    const SkPoint points[2] = {SkPoint::Make(left, y), SkPoint::Make(right, y)};
+    const SkColor4f line4 =
+        SkColor4f::FromColor(WithAlpha(line, splitter_opacity));
+    const SkColor4f clear = SkColors::kTransparent;
+    const SkColor4f stops[4] = {clear, line4, line4, clear};
+    const SkScalar positions[4] = {0.0f, 0.18f, 0.82f, 1.0f};
+    cc::PaintFlags flags;
+    flags.setShader(cc::PaintShader::MakeLinearGradient(
+        points, stops, positions, 4, SkTileMode::kClamp));
+    canvas->DrawRect(gfx::RectF(left, y, right - left, 1.0f), flags);
+  }
+  const bool rail = IsRail();
+  const float grip_width = rail ? 16.0f : 24.0f;
+  constexpr float kGripHeight = 4.0f;
+  const float grip_alpha =
+      (kGripRestAlpha +
+       (kGripActiveAlpha - kGripRestAlpha) *
+           static_cast<float>(grip_animation_.GetCurrentValue())) *
+      splitter_opacity;
+  cc::PaintFlags grip;
+  grip.setAntiAlias(true);
+  grip.setColor(WithAlpha(
+      GetColorProvider()->GetColor(kColorTabForegroundActiveFrameActive),
+      grip_alpha));
+  canvas->DrawRoundRect(
+      gfx::RectF((width() - grip_width) / 2.0f, center_y - kGripHeight / 2.0f,
+                 grip_width, kGripHeight),
+      kGripHeight / 2.0f, grip);
+}
+
+void WindriseAiSectionView::OnThemeChanged() {
+  views::View::OnThemeChanged();
+  SchedulePaint();
+}
+
+// --- 数据 -------------------------------------------------------------------
+
+std::vector<WindriseAiSectionView::RowData>
+WindriseAiSectionView::CollectRows() const {
+  std::vector<RowData> rows;
+  if (!tab_strip_model_ || !tab_strip_model_->group_model()) {
+    return rows;
+  }
+  const browseros::windrise::AiWindowState* ai_state =
+      browseros::windrise::AiWindowState::From(browser_);
+  if (!ai_state) {
+    return rows;
+  }
+  const browseros::windrise::AiState& state = ai_state->state();
+  const tabs::TabInterface* active_tab = tab_strip_model_->GetActiveTab();
+  TabGroupModel* group_model = tab_strip_model_->group_model();
+
+  // AiState.sections 按 AI 第一次出现的先后排；下段从下往上排，所以先来的
+  // AI 在最下面。每个 AI：先是组合页行，展开了再在它上面依次排它的标签。
+  for (const browseros::windrise::AiSection& section : state.sections) {
+    if (!section.group || !group_model->ContainsTabGroup(*section.group)) {
+      continue;
+    }
+    const TabGroup* group = group_model->GetTabGroup(*section.group);
+    const gfx::Range range = group->ListTabs();
+    if (range.is_empty()) {
+      continue;
+    }
+    std::vector<tabs::TabInterface*> tabs_in_group;
+    bool contains_active = false;
+    for (size_t i = range.start(); i < range.end(); ++i) {
+      tabs::TabInterface* tab =
+          tab_strip_model_->GetTabAtIndex(static_cast<int>(i));
+      if (!tab) {
+        continue;
+      }
+      contains_active |= tab == active_tab;
+      tabs_in_group.push_back(tab);
+    }
+    const auto expanded_it = expanded_.find(section.name);
+    const bool expanded =
+        expanded_it != expanded_.end() && expanded_it->second;
+
+    RowData header;
+    header.is_header = true;
+    header.ai_name = section.name;
+    header.tab_count = static_cast<int>(tabs_in_group.size());
+    header.expanded = expanded;
+    header.active = contains_active && !expanded;
+    // 「正在操作」：这个 AI 是最近一次推标签的那一个，并且扩展报告有标签正在
+    // 被 AI 操作（setAiActivity）。setAiActivity 不带 AI 名，多个 AI 同时
+    // 操作时只能标在最近那一个上（见交付稿「没做成的」）。
+    header.operating = section.name == state.active_ai_name &&
+                       !state.active_tab_ids.empty();
+    for (const browseros::windrise::AiConnection& connection :
+         state.connections) {
+      if (connection.name == section.name && !connection.icon_png.empty()) {
+        header.ai_icon =
+            gfx::Image::CreateFrom1xPNGBytes(
+                base::as_byte_span(connection.icon_png))
+                .AsImageSkia();
+        break;
+      }
+    }
+    rows.push_back(std::move(header));
+
+    if (!expanded) {
+      continue;
+    }
+    for (tabs::TabInterface* tab : tabs_in_group) {
+      RowData row;
+      row.ai_name = section.name;
+      row.tab = tab->GetHandle();
+      row.active = tab == active_tab;
+      rows.push_back(std::move(row));
+    }
+  }
+  return rows;
+}
+
+void WindriseAiSectionView::Rebuild(bool animate_height) {
+  std::vector<RowData> rows = CollectRows();
+  const int old_content = ContentHeight();
+  const bool was_empty = row_data_.empty();
+
+  bool same_shape = rows.size() == row_data_.size();
+  for (size_t i = 0; same_shape && i < rows.size(); ++i) {
+    same_shape = rows[i].is_header == row_data_[i].is_header &&
+                 rows[i].ai_name == row_data_[i].ai_name &&
+                 rows[i].tab == row_data_[i].tab;
+  }
+  row_data_ = std::move(rows);
+
+  if (same_shape) {
+    // 只有内容变了（当前标签、正在操作、数字）：原地更新，悬停状态不丢。
+    for (size_t i = 0; i < rows_.size(); ++i) {
+      rows_[i]->SetData(row_data_[i]);
+    }
+    return;
+  }
+
+  for (Row* row : rows_) {
+    viewport_->RemoveChildViewT(row);
+  }
+  rows_.clear();
+  for (size_t i = 0; i < row_data_.size(); ++i) {
+    Row* row = viewport_->AddChildViewAt(std::make_unique<Row>(this), i);
+    row->SetData(row_data_[i]);
+    rows_.push_back(row);
+  }
+  // 滚动细条永远在行的上面。
+  viewport_->ReorderChildView(indicator_, viewport_->children().size());
+
+  if (animate_height && (ContentHeight() != old_content || was_empty)) {
+    StartHeightTransition();
+  } else {
+    PreferredSizeChanged();
+  }
+  InvalidateLayout();
+}
+
+void WindriseAiSectionView::RefreshRowContents() {
+  // 标题、图标、提示文字、读屏名字都跟着标签走。
+  for (size_t i = 0; i < rows_.size() && i < row_data_.size(); ++i) {
+    rows_[i]->SetData(row_data_[i]);
+  }
+}
+
+void WindriseAiSectionView::OnAiStateChanged() {
+  Rebuild(/*animate_height=*/true);
+}
+
+void WindriseAiSectionView::ToggleExpanded(const std::u16string& ai_name) {
+  bool& expanded = expanded_[ai_name];
+  expanded = !expanded;
+  Rebuild(/*animate_height=*/true);
+}
+
+void WindriseAiSectionView::ActivateTab(tabs::TabInterface* tab) {
+  if (!tab_strip_model_ || !tab) {
+    return;
+  }
+  const int index = tab_strip_model_->GetIndexOfTab(tab);
+  if (index != TabStripModel::kNoTab) {
+    tab_strip_model_->ActivateTabAt(
+        index, TabStripUserGestureDetails(
+                   TabStripUserGestureDetails::GestureType::kMouse));
+  }
+}
+
+void WindriseAiSectionView::CloseTab(tabs::TabInterface* tab) {
+  if (!tab_strip_model_ || !tab) {
+    return;
+  }
+  const int index = tab_strip_model_->GetIndexOfTab(tab);
+  if (index != TabStripModel::kNoTab) {
+    tab_strip_model_->CloseWebContentsAt(
+        index, TabCloseTypes::CLOSE_USER_GESTURE);
+  }
+}
+
+void WindriseAiSectionView::OnRowPressed(Row* row, const ui::Event& event) {
+  const RowData& data = row->data();
+  bool close = false;
+  if (!data.is_header) {
+    if (event.IsMouseEvent() &&
+        (event.AsMouseEvent()->changed_button_flags() &
+         ui::EF_MIDDLE_MOUSE_BUTTON)) {
+      close = true;
+    } else if (event.IsLocatedEvent() && row->ShowsClose() &&
+               row->GetCloseHitRect().Contains(gfx::ToFlooredPoint(
+                   event.AsLocatedEvent()->location_f()))) {
+      close = true;
+    }
+  }
+  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
+      FROM_HERE, base::BindOnce(&WindriseAiSectionView::PerformRowAction,
+                                weak_factory_.GetWeakPtr(), data, close));
+}
+
+void WindriseAiSectionView::PerformRowAction(RowData data, bool close) {
+  if (data.is_header) {
+    ToggleExpanded(data.ai_name);
+    return;
+  }
+  tabs::TabInterface* tab = data.tab.Get();
+  if (close) {
+    CloseTab(tab);
+  } else {
+    ActivateTab(tab);
+  }
+}
+
+// --- 滚动：方向和上段相反 ----------------------------------------------------
+
+void WindriseAiSectionView::SetScrollOffset(int offset) {
+  const int clamped = std::clamp(offset, 0, MaxScrollOffset());
+  if (clamped == scroll_offset_) {
+    return;
+  }
+  scroll_offset_ = clamped;
+  InvalidateLayout();
+  ShowScrollIndicator();
+}
+
+void WindriseAiSectionView::ShowScrollIndicator() {
+  if (MaxScrollOffset() <= 0) {
+    return;
+  }
+  indicator_animation_.SetSlideDuration(
+      gfx::Animation::RichAnimationDuration(kIndicatorInDuration));
+  indicator_animation_.Show();
+  indicator_->SchedulePaint();
+  indicator_hide_timer_.Start(
+      FROM_HERE, kIndicatorHold,
+      base::BindOnce(
+          [](WindriseAiSectionView* view) {
+            view->indicator_animation_.SetSlideDuration(
+                gfx::Animation::RichAnimationDuration(kIndicatorOutDuration));
+            view->indicator_animation_.Hide();
+          },
+          base::Unretained(this)));
+}
+
+bool WindriseAiSectionView::OnMouseWheel(const ui::MouseWheelEvent& event) {
+  if (MaxScrollOffset() <= 0 || event.y_offset() == 0) {
+    return false;
+  }
+  // 和上段同一个方向（R189「就正常滚动」）：滚轮往下，内容往上走，露出下沿
+  // 以下的；滚轮往上，露出上面（更晚开的）标签。y_offset 往下滚是负数；
+  // scroll_offset_ 是藏在下沿以下的高度。
+  SetScrollOffset(scroll_offset_ + kWheelDirection * event.y_offset());
+  return true;
+}
+
+void WindriseAiSectionView::OnScrollEvent(ui::ScrollEvent* event) {
+  // 触控板两指滚动（非 Windows 平台走这里；Windows 的精确触控板走上面的
+  // 滚轮事件），和滚轮同一个方向约定。
+  if (event->type() != ui::EventType::kScroll || MaxScrollOffset() <= 0 ||
+      event->y_offset() == 0) {
+    return;
+  }
+  SetScrollOffset(scroll_offset_ +
+                  kWheelDirection * base::ClampRound(event->y_offset()));
+  event->SetHandled();
+}
+
+void WindriseAiSectionView::OnGestureEvent(ui::GestureEvent* event) {
+  // 触屏拖动是直接操作：手指往下拖，内容跟着往下走（露出上面的）。
+  switch (event->type()) {
+    case ui::EventType::kGestureScrollBegin:
+      if (MaxScrollOffset() > 0) {
+        event->SetHandled();
+      }
+      break;
+    case ui::EventType::kGestureScrollUpdate:
+      if (MaxScrollOffset() > 0) {
+        SetScrollOffset(scroll_offset_ +
+                        base::ClampRound(event->details().scroll_y()));
+        event->SetHandled();
+      }
+      break;
+    default:
+      break;
+  }
+}
+
+// --- 拖分隔条 ---------------------------------------------------------------
+
+bool WindriseAiSectionView::IsInSplitter(const gfx::Point& point) const {
+  return !row_data_.empty() && point.y() >= 0 &&
+         point.y() < SplitterHeight() && point.x() >= 0 &&
+         point.x() < width();
+}
+
+ui::Cursor WindriseAiSectionView::GetCursor(const ui::MouseEvent& event) {
+  if ((drag_height_ || IsInSplitter(event.location())) &&
+      ComputeAllocation().colliding) {
+    return ui::Cursor(ui::mojom::CursorType::kRowResize);
+  }
+  return views::View::GetCursor(event);
+}
+
+void WindriseAiSectionView::UpdateSplitterHover(bool hovered) {
+  splitter_hovered_ = hovered;
+  if (hovered || pressed_in_splitter_) {
+    grip_animation_.Show();
+  } else {
+    grip_animation_.Hide();
+  }
+}
+
+void WindriseAiSectionView::OnMouseMoved(const ui::MouseEvent& event) {
+  UpdateSplitterHover(IsInSplitter(event.location()) &&
+                      ComputeAllocation().colliding);
+}
+
+void WindriseAiSectionView::OnMouseExited(const ui::MouseEvent& event) {
+  UpdateSplitterHover(false);
+}
+
+bool WindriseAiSectionView::OnMousePressed(const ui::MouseEvent& event) {
+  if (!event.IsOnlyLeftMouseButton() || !IsInSplitter(event.location()) ||
+      !ComputeAllocation().colliding) {
+    return false;
+  }
+  // 抓住的是眼前的高：正在回弹 / 过渡也立刻停在当前值，从这里跟手。
+  height_animation_.Stop();
+  pressed_in_splitter_ = true;
+  drag_started_ = false;
+  drag_start_height_ = height();
+  drag_height_ = drag_start_height_;
+  // 用根视图坐标：拖的时候本视图的上沿自己在动，本地坐标会跟着漂。
+  drag_start_y_ = event.root_location().y();
+  UpdateSplitterHover(true);
+  return true;
+}
+
+bool WindriseAiSectionView::OnMouseDragged(const ui::MouseEvent& event) {
+  if (!pressed_in_splitter_) {
+    return false;
+  }
+  // 往上拖 = 下段变高。
+  const int delta = drag_start_y_ - event.root_location().y();
+  if (!drag_started_ && std::abs(delta) < kDragThreshold) {
+    return true;
+  }
+  drag_started_ = true;
+  // 1:1 跟手（含抓住的位置）；超过上下限以后越拖越沉（橡皮筋），松手弹回。
+  const Allocation allocation = ComputeAllocation();
+  const int raw = drag_start_height_ + delta;
+  int height_now = raw;
+  if (raw > allocation.max_height) {
+    height_now = allocation.max_height +
+                 RubberBand(raw - allocation.max_height, allocation.available);
+  } else if (raw < allocation.min_height) {
+    height_now = allocation.min_height -
+                 RubberBand(allocation.min_height - raw, allocation.available);
+  }
+  drag_height_ = std::max(0, height_now);
+  PreferredSizeChanged();
+  return true;
+}
+
+void WindriseAiSectionView::FinishDrag() {
+  pressed_in_splitter_ = false;
+  if (drag_height_ && drag_started_) {
+    // 记住松手的位置：按「下段占两段可用高的比例」存，窗口高度变了也照比例
+    // 分。拖过了上下限的部分不算（橡皮筋只是手感）。
+    const Allocation allocation = ComputeAllocation();
+    const int committed = std::clamp(*drag_height_, allocation.min_height,
+                                     allocation.max_height);
+    const int splitter = SplitterHeight();
+    const double ratio = static_cast<double>(committed - splitter) /
+                         std::max(1, allocation.available - splitter);
+    if (prefs_ &&
+        prefs_->FindPreference(browseros::windrise::kAiSectionRatioPref)) {
+      prefs_->SetDouble(browseros::windrise::kAiSectionRatioPref,
+                        std::clamp(ratio, kMinRatio, kMaxRatio));
+    }
+  }
+  drag_started_ = false;
+  if (drag_height_) {
+    // 从眼前（可能是橡皮筋拉出来的）高度过渡到目标：拖过边界就弹回去；按下
+    // 没拖就接着走完被按住之前的过渡。
+    drag_height_.reset();
+    StartHeightTransition();
+  }
+}
+
+void WindriseAiSectionView::OnMouseReleased(const ui::MouseEvent& event) {
+  if (!pressed_in_splitter_) {
+    return;
+  }
+  FinishDrag();
+  UpdateSplitterHover(IsInSplitter(event.location()) &&
+                      ComputeAllocation().colliding);
+}
+
+void WindriseAiSectionView::OnMouseCaptureLost() {
+  // 被系统打断（切窗口等）：按松手处理，停在当前位置。
+  if (!pressed_in_splitter_) {
+    return;
+  }
+  FinishDrag();
+  UpdateSplitterHover(false);
+}
+
+// --- 观察 -------------------------------------------------------------------
+
+void WindriseAiSectionView::OnTabStripModelChanged(
+    TabStripModel* tab_strip_model,
+    const TabStripModelChange& change,
+    const TabStripSelectionChange& selection) {
+  Rebuild(/*animate_height=*/true);
+}
+
+void WindriseAiSectionView::OnTabChangedAt(tabs::TabInterface* tab,
+                                           TabChangeType change_type) {
+  for (const RowData& row : row_data_) {
+    if (!row.is_header && row.tab.Get() == tab) {
+      RefreshRowContents();
+      return;
+    }
+  }
+}
+
+void WindriseAiSectionView::OnTabGroupChanged(const TabGroupChange& change) {
+  Rebuild(/*animate_height=*/true);
+}
+
+void WindriseAiSectionView::OnTabStripModelDestroyed(
+    TabStripModel* tab_strip_model) {
+  if (tab_strip_model == tab_strip_model_) {
+    tab_strip_model_->RemoveObserver(this);
+    tab_strip_model_ = nullptr;
+    Rebuild(/*animate_height=*/false);
+  }
+}
+
+void WindriseAiSectionView::OnViewPreferredSizeChanged(
+    views::View* observed_view) {
+  // 上段长高 / 变矮：段高不一定变，但「碰没碰到」可能变，要重新排一次。
+  InvalidateLayout();
+}
+
+void WindriseAiSectionView::OnViewBoundsChanged(views::View* observed_view) {
+  if (observed_view == parent()) {
+    InvalidateLayout();
+  }
+}
+
+void WindriseAiSectionView::OnViewIsDeleting(views::View* observed_view) {
+  observations_.RemoveObservation(observed_view);
+  if (observed_view == tab_strip_) {
+    tab_strip_ = nullptr;
+  }
+  if (observed_view == new_tab_container_) {
+    new_tab_container_ = nullptr;
+  }
+}
+
+void WindriseAiSectionView::AnimationProgressed(
+    const gfx::Animation* animation) {
+  if (animation == &height_animation_) {
+    PreferredSizeChanged();
+  } else if (animation == &indicator_animation_) {
+    indicator_->SchedulePaint();
+  } else {
+    SchedulePaint();
+  }
+}
+
+void WindriseAiSectionView::AnimationEnded(const gfx::Animation* animation) {
+  AnimationProgressed(animation);
+}
+
+BEGIN_METADATA(WindriseAiSectionView)
+END_METADATA
