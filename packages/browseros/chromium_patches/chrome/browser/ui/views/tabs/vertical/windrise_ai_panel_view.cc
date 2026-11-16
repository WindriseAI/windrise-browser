diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc
new file mode 100644
index 0000000000..9b8d9a47af
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc
@@ -0,0 +1,494 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h"
+
+#include <memory>
+#include <utility>
+
+#include "base/functional/bind.h"
+#include "base/i18n/number_formatting.h"
+#include "base/i18n/time_formatting.h"
+#include "base/time/time.h"
+#include "cc/paint/paint_flags.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/profiles/profile.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
+#include "chrome/grit/generated_resources.h"
+#include "extensions/browser/event_router.h"
+#include "extensions/browser/extension_event_histogram_value.h"
+#include "third_party/skia/include/core/SkRRect.h"
+#include "ui/base/l10n/l10n_util.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/base/mojom/dialog_button.mojom.h"
+#include "ui/color/color_provider.h"
+#include "ui/color/color_variant.h"
+#include "ui/compositor/layer.h"
+#include "ui/compositor/scoped_layer_animation_settings.h"
+#include "ui/gfx/canvas.h"
+#include "ui/gfx/color_utils.h"
+#include "ui/gfx/font.h"
+#include "ui/gfx/font_list.h"
+#include "ui/gfx/animation/animation.h"
+#include "ui/gfx/animation/tween.h"
+#include "ui/gfx/geometry/transform.h"
+#include "ui/gfx/geometry/transform_util.h"
+#include "ui/gfx/geometry/skia_conversions.h"
+#include "ui/gfx/skia_paint_util.h"
+#include "ui/views/accessibility/view_accessibility.h"
+#include "ui/views/background.h"
+#include "ui/views/border.h"
+#include "ui/views/bubble/bubble_border.h"
+#include "ui/views/controls/button/label_button.h"
+#include "ui/views/controls/highlight_path_generator.h"
+#include "ui/views/controls/label.h"
+#include "ui/views/controls/separator.h"
+#include "ui/views/layout/box_layout.h"
+#include "ui/views/layout/box_layout_view.h"
+#include "ui/views/style/typography.h"
+#include "ui/views/style/typography_provider.h"
+#include "ui/views/view_class_properties.h"
+#include "ui/views/widget/widget.h"
+
+namespace {
+
+using browseros::windrise::LayoutMetric;
+
+constexpr char kPanelActionEventName[] = "browserOS.onAiPanelAction";
+
+// 面板按钮发给扩展的 action 名（扩展 sw.js forwardGridAction 分发）。
+constexpr char kActionConnectAi[] = "connect_ai";
+constexpr char kActionOpenApp[] = "open_app";
+// 原来叫 "grid"，和工具栏格子按钮撞名；扩展两个名字都认（sw.js:1693）。
+constexpr char kActionGallery[] = "gallery";
+constexpr char kActionSettings[] = "settings";
+
+// 定稿帧（ntp-zh-panel / ntp-en-panel，2x 图按 1/2 量）：
+// 「接一个 AI」按钮 36 高（偏好 ai_panel_button_height）、圆角 10、字 14
+// Medium，底 rgb(255 255 255 / .78)；底部文字行字 14，线到字顶 10；
+// 「闪办广场」下面的小字 11。
+constexpr int kTextSize = 14;
+constexpr int kCaptionSize = 11;
+constexpr int kConnectButtonRadius = 10;
+constexpr SkAlpha kConnectButtonAlpha = 0xC7;  // .78
+constexpr int kSectionGap = 10;
+constexpr int kLineGap = 12;  // 「接一个 AI」下沿到线
+// 文字按钮的命中区比字大一圈：上下 4、左右 6；悬停 / 按下的底是这一圈圆角 6。
+// 底部一行整体向左右各伸出 6，字仍和面板内边 14 对齐。
+constexpr int kTextButtonInsetV = 4;
+constexpr int kTextButtonInsetH = 6;
+constexpr int kTextButtonRadius = 6;
+// 两个文字之间看上去的间距 14（漏做核查 L7 ③）= 按钮间距 2 + 两边各 6。
+constexpr int kFooterTextGap = 14;
+// 悬停 / 按下：在底色上叠墨色 4% / 8%（和卡片同一套，windrise_ai_card_view.cc
+// 的 10 / 20）。
+constexpr SkAlpha kHoverInkAlpha = 10;
+constexpr SkAlpha kPressedInkAlpha = 20;
+// 按下缩到 .97（emil-design-eng「Buttons must feel responsive」）。按下快、
+// 松开稍慢（review-animations 第 9 条：按 / 松不对称）；曲线 WINDRISE_EASE_OUT，
+// 合成器逐帧推进（120Hz 屏 120 帧，已定 甲52）；中途松开从当前值反向，不跳。
+// 系统关了动画（Windows「显示动画」= 减少动态效果）时一帧到位，颜色反馈照旧。
+constexpr float kPressedScale = 0.97f;
+constexpr base::TimeDelta kPressDuration = base::Milliseconds(100);
+constexpr base::TimeDelta kReleaseDuration = base::Milliseconds(160);
+
+int Metric(LayoutMetric metric) {
+  return browseros::windrise::GetLayoutMetrics().Get(metric);
+}
+
+gfx::FontList SizedFont(const gfx::FontList& base, int size, bool medium) {
+  gfx::FontList font = base.DeriveWithSizeDelta(size - base.GetFontSize());
+  return medium ? font.DeriveWithWeight(gfx::Font::Weight::MEDIUM) : font;
+}
+
+std::unique_ptr<views::Label> MakeLabel(const std::u16string& text,
+                                        ui::ColorId color_id,
+                                        bool medium) {
+  auto label = std::make_unique<views::Label>(text);
+  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
+  label->SetEnabledColor(ui::ColorVariant(color_id));
+  if (medium) {
+    label->SetFontList(views::TypographyProvider::Get().GetFont(
+        views::style::CONTEXT_DIALOG_TITLE, views::style::STYLE_PRIMARY));
+  }
+  return label;
+}
+
+// 帧里「2 分钟前」「昨天 16:12」：一小时内写「N 分钟前」，今天写「今天 HH:MM」，
+// 昨天写「昨天 HH:MM」，更早写「M 月 D 日」（英文 Today 16:12 / Yesterday
+// 16:12 / 11/14）。两张帧都是 24 小时制，中英文都按 24 小时写。扩展现在只推
+// 「距今分钟数」，绝对时间 = 现在 - 分钟数；今天 / 昨天按本地零点分。
+std::u16string FormatLastActive(int minutes) {
+  if (minutes < 60) {
+    return l10n_util::GetStringFUTF16(IDS_WINDRISE_AI_MINUTES_AGO,
+                                      base::FormatNumber(minutes));
+  }
+  const base::Time now = base::Time::Now();
+  const base::Time when = now - base::Minutes(minutes);
+  const base::Time today = now.LocalMidnight();
+  // 往前退 12 小时再取零点：夏令时切换那天也落在昨天的零点上。
+  const base::Time yesterday = (today - base::Hours(12)).LocalMidnight();
+  if (when >= yesterday) {
+    const std::u16string clock = base::TimeFormatTimeOfDayWithHourClockType(
+        when, base::k24HourClock, base::kDropAmPm);
+    return l10n_util::GetStringFUTF16(
+        when >= today ? IDS_WINDRISE_AI_TODAY_AT : IDS_WINDRISE_AI_YESTERDAY_AT,
+        clock);
+  }
+  base::Time::Exploded exploded;
+  when.LocalExplode(&exploded);
+  return l10n_util::GetStringFUTF16(IDS_WINDRISE_AI_MONTH_DAY,
+                                    base::FormatNumber(exploded.month),
+                                    base::FormatNumber(exploded.day_of_month));
+}
+
+// 面板里的两种按钮：kFilled =「接一个 AI」白底宽按钮；kText = 底部文字行。
+// 反馈（emil-design-eng：可按的东西都要有按下反馈，不加别的动效）：悬停叠
+// 墨色 4%；按下叠 8% 并缩到 .97；键盘焦点走系统焦点环（形状同底的圆角）。
+// 不可用（闪办广场，完整版即将上线）时不画悬停 / 按下底，字用次要色。
+class PanelButton : public views::LabelButton {
+  METADATA_HEADER(PanelButton, views::LabelButton)
+
+ public:
+  enum class Style { kFilled, kText };
+
+  PanelButton(PressedCallback callback,
+              const std::u16string& text,
+              Style style)
+      : views::LabelButton(std::move(callback), text), style_(style) {
+    const bool filled = style_ == Style::kFilled;
+    label()->SetFontList(
+        SizedFont(label()->font_list(), kTextSize, /*medium=*/filled));
+    SetEnabledTextColors(
+        ui::ColorVariant(kColorTabForegroundActiveFrameActive));
+    SetTextColor(views::Button::STATE_DISABLED,
+                 ui::ColorVariant(kColorTabForegroundInactiveFrameActive));
+    radius_ = filled ? kConnectButtonRadius : kTextButtonRadius;
+    if (filled) {
+      SetHorizontalAlignment(gfx::ALIGN_CENTER);
+      SetMinSize(gfx::Size(0, Metric(LayoutMetric::kAiPanelButtonHeight)));
+      SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(0, 12)));
+    } else {
+      SetHorizontalAlignment(gfx::ALIGN_LEFT);
+      SetBorder(views::CreateEmptyBorder(
+          gfx::Insets::VH(kTextButtonInsetV, kTextButtonInsetH)));
+    }
+    SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
+    SetInstallFocusRingOnFocus(true);
+    views::InstallRoundRectHighlightPathGenerator(this, gfx::Insets(),
+                                                  radius_);
+    // 按下缩放走自己的层（只动 transform，不重排、不重画）。
+    SetPaintToLayer();
+    layer()->SetFillsBoundsOpaquely(false);
+  }
+  PanelButton(const PanelButton&) = delete;
+  PanelButton& operator=(const PanelButton&) = delete;
+  ~PanelButton() override = default;
+
+  // views::Button:
+  void StateChanged(ButtonState old_state) override {
+    views::LabelButton::StateChanged(old_state);
+    const bool pressed = GetState() == STATE_PRESSED;
+    if (pressed == (old_state == STATE_PRESSED)) {
+      return;
+    }
+    ui::ScopedLayerAnimationSettings settings(layer()->GetAnimator());
+    settings.SetPreemptionStrategy(
+        ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET);
+    settings.SetTransitionDuration(
+        gfx::Animation::ShouldRenderRichAnimation()
+            ? (pressed ? kPressDuration : kReleaseDuration)
+            : base::TimeDelta());
+    settings.SetTweenType(gfx::Tween::WINDRISE_EASE_OUT);
+    layer()->SetTransform(
+        pressed ? gfx::GetScaleTransform(GetLocalBounds().CenterPoint(),
+                                         kPressedScale)
+                : gfx::Transform());
+  }
+
+  // views::View:
+  void OnPaintBackground(gfx::Canvas* canvas) override {
+    const ui::ColorProvider* color_provider = GetColorProvider();
+    if (!color_provider) {
+      return;
+    }
+    const SkRRect rrect = SkRRect::MakeRectXY(
+        gfx::RectToSkRect(GetLocalBounds()), radius_, radius_);
+    cc::PaintFlags flags;
+    flags.setAntiAlias(true);
+    flags.setStyle(cc::PaintFlags::kFill_Style);
+    if (style_ == Style::kFilled) {
+      flags.setColor(SkColorSetA(
+          color_provider->GetColor(kColorWindriseAiPanelButtonBackground),
+          kConnectButtonAlpha));
+      canvas->sk_canvas()->drawRRect(rrect, flags);
+    }
+    if (!GetEnabled()) {
+      return;
+    }
+    const ButtonState state = GetState();
+    if (state != STATE_HOVERED && state != STATE_PRESSED) {
+      return;
+    }
+    flags.setColor(SkColorSetA(
+        color_provider->GetColor(kColorTabForegroundActiveFrameActive),
+        state == STATE_PRESSED ? kPressedInkAlpha : kHoverInkAlpha));
+    canvas->sk_canvas()->drawRRect(rrect, flags);
+  }
+
+ private:
+  const Style style_;
+  int radius_ = 0;
+};
+
+BEGIN_METADATA(PanelButton)
+END_METADATA
+
+}  // namespace
+
+WindriseAiPanelView::WindriseAiPanelView(BrowserWindowInterface* browser)
+    // 158 里 BubbleDialogDelegateView 的常规构造函数在 private 段（friend 表），
+    // 新建子类走公开的 PassKey 构造。
+    : views::BubbleDialogDelegateView(
+          views::BubbleDialogDelegateView::CreatePassKey(),
+          nullptr,
+          views::BubbleBorder::BOTTOM_RIGHT),
+      browser_(browser) {
+  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
+  set_close_on_deactivate(true);
+  // B 版银色玻璃（已定 甲08、漏做核查 L7）：渐变、高光由 OnPaintBackground
+  // 画满整个面板；气泡框自己的底取渐变下端色，圆角抗锯齿那一圈不露别的颜色。
+  // 外面那条 .5px 线和一层投影用气泡框自带的（BubbleBorder 画 1 物理像素的
+  // 线 + 投影），这里不再画第二条。
+  SetBackgroundColor(ui::ColorVariant(kColorWindriseAiPanelBackgroundBottom));
+  set_corner_radius(Metric(LayoutMetric::kAiPanelRadius));
+  // 内边放进自己的 border（set_margins 的内边在本视图外面，渐变铺不到）。
+  set_margins(gfx::Insets());
+  SetBorder(views::CreateEmptyBorder(
+      gfx::Insets(Metric(LayoutMetric::kAiPanelPadding))));
+
+  auto layout = std::make_unique<views::BoxLayout>(
+      views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0);
+  layout->set_cross_axis_alignment(
+      views::BoxLayout::CrossAxisAlignment::kStretch);
+  SetLayoutManager(std::move(layout));
+
+  rows_container_ = AddChildView(std::make_unique<views::BoxLayoutView>());
+  rows_container_->SetOrientation(views::BoxLayout::Orientation::kVertical);
+  rows_container_->SetBetweenChildSpacing(0);
+
+  // 「接一个 AI」：行下面的白底宽按钮（两张帧；方案 N7「36 高圆角 10、底
+  // rgb(255 255 255 / .78)」）。放在 rows_container_ 外面：扩展推新数据重建行
+  // 时它不跟着重建，悬停、焦点不丢。
+  auto connect = std::make_unique<PanelButton>(
+      base::BindRepeating(&WindriseAiPanelView::NotifyAction,
+                          base::Unretained(this), kActionConnectAi),
+      l10n_util::GetStringUTF16(IDS_WINDRISE_AI_CONNECT),
+      PanelButton::Style::kFilled);
+  connect->SetProperty(views::kMarginsKey,
+                       gfx::Insets::TLBR(kSectionGap, 0, 0, 0));
+  AddChildView(std::move(connect));
+
+  // 按钮下面一条 .5px 线（帧），和上面行间的线同一种。
+  auto separator = std::make_unique<views::Separator>();
+  separator->SetColorId(kColorWindriseDivider);
+  separator->SetPreferredSize(gfx::Size(0, 1));
+  separator->SetProperty(views::kMarginsKey,
+                         gfx::Insets::TLBR(kLineGap, 0, 0, 0));
+  AddChildView(std::move(separator));
+
+  // 底部文字行：「打开风桥」「闪办广场」靠左，「设置」靠右（L7 ③、两张帧）。
+  // 线到字顶 10：文字按钮自带上内边 4，所以这一行上边距 10 - 4；左右、下面各
+  // 伸出按钮自带的内边，字和上面的内容在同一条竖线上，底边距仍是面板内边。
+  auto* footer = AddChildView(std::make_unique<views::BoxLayoutView>());
+  footer->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
+  footer->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kStart);
+  footer->SetBetweenChildSpacing(kFooterTextGap - 2 * kTextButtonInsetH);
+  footer->SetProperty(
+      views::kMarginsKey,
+      gfx::Insets::TLBR(kSectionGap - kTextButtonInsetV, -kTextButtonInsetH,
+                        -kTextButtonInsetV, -kTextButtonInsetH));
+
+  const auto make_text_button = [this](int string_id, const char* action) {
+    return std::make_unique<PanelButton>(
+        base::BindRepeating(&WindriseAiPanelView::NotifyAction,
+                            base::Unretained(this), action),
+        l10n_util::GetStringUTF16(string_id), PanelButton::Style::kText);
+  };
+  footer->AddChildView(
+      make_text_button(IDS_WINDRISE_AI_PANEL_OPEN_APP, kActionOpenApp));
+
+  // 闪办广场（用户 11-16 15:37：「闪办宣传也是一个完整版即将上线的这个一个口径」）：
+  // 保留位置，但完整广场还没随包（主程序返回 GALLERY_UNAVAILABLE），所以做成
+  // 「还不能点」：按钮不可用（没有悬停 / 按下底、不进 Tab 顺序、字用次要色），
+  // 下面一行小字「完整版即将上线」，读屏读出名字加「完整版即将上线」。
+  // 广场页随包以后去掉 SetEnabled(false) 和小字即可，action 已经接好。
+  auto* gallery_column =
+      footer->AddChildView(std::make_unique<views::BoxLayoutView>());
+  gallery_column->SetOrientation(views::BoxLayout::Orientation::kVertical);
+  gallery_column->SetCrossAxisAlignment(
+      views::BoxLayout::CrossAxisAlignment::kStart);
+  const std::u16string coming_soon =
+      l10n_util::GetStringUTF16(IDS_WINDRISE_AI_GALLERY_SOON);
+  auto* gallery = gallery_column->AddChildView(
+      make_text_button(IDS_WINDRISE_AI_GALLERY, kActionGallery));
+  gallery->SetEnabled(false);
+  gallery->GetViewAccessibility().SetDescription(coming_soon);
+  auto* caption = gallery_column->AddChildView(MakeLabel(
+      coming_soon, kColorTabForegroundInactiveFrameActive, /*medium=*/false));
+  caption->SetFontList(
+      SizedFont(caption->font_list(), kCaptionSize, /*medium=*/false));
+  // 小字和「闪办广场」左对齐，紧贴上一行（按钮自带下内边 4，这里收回 2）。
+  caption->SetProperty(
+      views::kMarginsKey,
+      gfx::Insets::TLBR(-2, kTextButtonInsetH, kTextButtonInsetV, 0));
+  // 名字和说明已经由按钮读出，小字本身不再单独读一遍。
+  caption->GetViewAccessibility().SetIsIgnored(true);
+
+  views::View* spacer = footer->AddChildView(std::make_unique<views::View>());
+  footer->SetFlexForView(spacer, 1, true);
+  footer->AddChildView(
+      make_text_button(IDS_WINDRISE_AI_SETTINGS, kActionSettings));
+
+  if (browseros::windrise::AiWindowState* state =
+          browseros::windrise::AiWindowState::From(browser_.get())) {
+    state_subscription_ = state->AddObserver(base::BindRepeating(
+        &WindriseAiPanelView::RebuildContent, base::Unretained(this)));
+  }
+  RebuildContent();
+}
+
+WindriseAiPanelView::~WindriseAiPanelView() = default;
+
+void WindriseAiPanelView::ShowAtAnchor(views::View* anchor, bool collapsed) {
+  if (!anchor) {
+    return;
+  }
+  SetAnchorView(anchor);
+  // 展开：面板在卡片正上方、左边对齐卡片（宽 224）；
+  // 折叠：卡片只有 54 宽，面板从它右边浮出（宽 280），底边对齐卡片。
+  SetArrow(collapsed ? views::BubbleBorder::LEFT_BOTTOM
+                     : views::BubbleBorder::BOTTOM_LEFT);
+  set_fixed_width(collapsed ? Metric(LayoutMetric::kAiPanelCollapsedWidth)
+                            : Metric(LayoutMetric::kAiPanelWidth));
+  views::Widget* widget =
+      views::BubbleDialogDelegateView::CreateBubble(this);
+  widget->Show();
+}
+
+void WindriseAiPanelView::OnPaintBackground(gfx::Canvas* canvas) {
+  const ui::ColorProvider* color_provider = GetColorProvider();
+  if (!color_provider) {
+    return;
+  }
+  // 和卡片同一套银色玻璃（windrise_ai_card_view.cc OnPaintBackground；方案 N7
+  // 面板定值）：上浅下深的竖向渐变 + 上缘 1px 白高光 .95 + 内圈 .75px 白 .6。
+  // 暗色下底是深色两档，高光收弱，不出现一圈亮白边。
+  const float radius =
+      static_cast<float>(Metric(LayoutMetric::kAiPanelRadius));
+  const SkColor top =
+      color_provider->GetColor(kColorWindriseAiPanelBackgroundTop);
+  const SkColor bottom =
+      color_provider->GetColor(kColorWindriseAiPanelBackgroundBottom);
+  const bool dark = color_utils::IsDark(bottom);
+  const SkRect bounds = gfx::RectToSkRect(GetLocalBounds());
+
+  cc::PaintFlags fill;
+  fill.setAntiAlias(true);
+  fill.setStyle(cc::PaintFlags::kFill_Style);
+  fill.setColor(bottom);
+  fill.setShader(gfx::CreateGradientShader(
+      gfx::Point(0, 0), gfx::Point(0, height()), top, bottom));
+  canvas->sk_canvas()->drawRRect(SkRRect::MakeRectXY(bounds, radius, radius),
+                                 fill);
+
+  cc::PaintFlags ring;
+  ring.setAntiAlias(true);
+  ring.setStyle(cc::PaintFlags::kStroke_Style);
+  ring.setStrokeWidth(0.75f);
+  ring.setColor(SkColorSetA(SK_ColorWHITE, dark ? 0x14 : 0x99));  // .6
+  canvas->sk_canvas()->drawRRect(
+      SkRRect::MakeRectXY(bounds.makeInset(0.375f, 0.375f), radius - 0.375f,
+                          radius - 0.375f),
+      ring);
+
+  cc::PaintFlags highlight;
+  highlight.setAntiAlias(true);
+  highlight.setStyle(cc::PaintFlags::kStroke_Style);
+  highlight.setStrokeWidth(1.0f);
+  highlight.setColor(SkColorSetA(SK_ColorWHITE, dark ? 0x26 : 0xF2));  // .95
+  canvas->sk_canvas()->drawLine(radius, 0.5f, width() - radius, 0.5f,
+                                highlight);
+}
+
+void WindriseAiPanelView::RebuildContent() {
+  rows_container_->RemoveAllChildViews();
+
+  browseros::windrise::AiWindowState* state =
+      browseros::windrise::AiWindowState::From(browser_.get());
+  const browseros::windrise::AiState* ai_state = state ? &state->state() : nullptr;
+  const size_t count = ai_state ? ai_state->connections.size() : 0;
+
+  const std::u16string title =
+      count == 0
+          ? l10n_util::GetStringUTF16(IDS_WINDRISE_AI_NONE_CONNECTED)
+          : l10n_util::GetStringFUTF16(
+                IDS_WINDRISE_AI_CONNECTED_COUNT,
+                base::FormatNumber(static_cast<int>(count)));
+  rows_container_->AddChildView(MakeLabel(
+      title, kColorTabForegroundActiveFrameActive, /*medium=*/true));
+
+  const int row_height = Metric(LayoutMetric::kAiPanelRowHeight);
+  for (size_t i = 0; i < count; ++i) {
+    const browseros::windrise::AiConnection& connection =
+        ai_state->connections[i];
+    auto* row = rows_container_->AddChildView(std::make_unique<views::BoxLayoutView>());
+    row->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
+    // 行高走偏好（BoxLayoutView 没有 SetMinSize，用最小交叉轴尺寸）。
+    row->SetMinimumCrossAxisSize(row_height);
+    row->SetCrossAxisAlignment(
+        views::BoxLayout::CrossAxisAlignment::kCenter);
+    views::Label* name = row->AddChildView(MakeLabel(
+        connection.name, kColorTabForegroundActiveFrameActive,
+        /*medium=*/false));
+    views::View* spacer = row->AddChildView(std::make_unique<views::View>());
+    row->SetFlexForView(spacer, 1, true);
+    if (connection.last_active_minutes >= 0) {
+      row->AddChildView(MakeLabel(
+          FormatLastActive(connection.last_active_minutes),
+          kColorTabForegroundInactiveFrameActive, /*medium=*/false));
+    }
+    (void)name;
+
+    auto separator = std::make_unique<views::Separator>();
+    separator->SetColorId(kColorWindriseDivider);
+    separator->SetPreferredSize(gfx::Size(0, 1));
+    rows_container_->AddChildView(std::move(separator));
+  }
+
+  // 面板已经开出来时，扩展再推新数据要让气泡跟着长高：BubbleDialogDelegate::
+  // SizeToContents() 会把边框 / 箭头也算进窗口尺寸并重新定位。
+  if (GetWidget()) {
+    SizeToContents();
+  }
+}
+
+void WindriseAiPanelView::NotifyAction(const char* action) {
+  Profile* profile = browser_ ? browser_->GetProfile() : nullptr;
+  if (!profile) {
+    return;
+  }
+  auto* event_router = extensions::EventRouter::Get(profile);
+  if (!event_router) {
+    return;
+  }
+  auto args = base::MakeRefCounted<extensions::EventArgs>();
+  args->data.Append(action);
+  event_router->BroadcastEvent(std::make_unique<extensions::Event>(
+      extensions::events::UNKNOWN, kPanelActionEventName, std::move(args),
+      profile));
+}
+
+BEGIN_METADATA(WindriseAiPanelView)
+END_METADATA
\ No newline at end of file
