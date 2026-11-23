diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc
new file mode 100644
index 0000000000..80f4944f06
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc
@@ -0,0 +1,624 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h"
+
+#include <algorithm>
+#include <array>
+#include <memory>
+#include <string_view>
+#include <utility>
+
+#include "base/containers/span.h"
+#include "base/functional/bind.h"
+#include "base/i18n/number_formatting.h"
+#include "base/i18n/rtl.h"
+#include "base/i18n/time_formatting.h"
+#include "base/memory/ref_counted.h"
+#include "base/strings/string_util.h"
+#include "base/time/time.h"
+#include "cc/paint/paint_flags.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/extensions/windrise_extension_protection.h"
+#include "chrome/browser/profiles/profile.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
+#include "chrome/browser/ui/singleton_tabs.h"
+#include "chrome/browser/ui/toasts/api/toast_id.h"
+#include "chrome/browser/ui/toasts/toast_controller.h"
+#include "chrome/common/webui_url_constants.h"
+#include "chrome/grit/generated_resources.h"
+#include "extensions/browser/event_router.h"
+#include "extensions/browser/extension_event_histogram_value.h"
+#include "extensions/browser/extension_registry.h"
+#include "third_party/skia/include/core/SkBitmap.h"
+#include "third_party/skia/include/core/SkRRect.h"
+#include "ui/base/l10n/l10n_util.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/base/mojom/dialog_button.mojom.h"
+#include "ui/color/color_provider.h"
+#include "ui/color/color_variant.h"
+#include "ui/compositor/layer.h"
+#include "ui/compositor/scoped_layer_animation_settings.h"
+#include "ui/gfx/canvas.h"
+#include "ui/gfx/codec/png_codec.h"
+#include "ui/gfx/image/image_skia.h"
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
+#include "ui/views/controls/focus_ring.h"
+#include "ui/views/controls/highlight_path_generator.h"
+#include "ui/views/controls/image_view.h"
+#include "ui/views/controls/label.h"
+#include "ui/views/controls/separator.h"
+#include "ui/views/layout/box_layout.h"
+#include "ui/views/layout/box_layout_view.h"
+#include "ui/views/style/typography.h"
+#include "ui/views/style/typography_provider.h"
+#include "ui/views/view_class_properties.h"
+#include "ui/views/widget/widget.h"
+#include "url/gurl.h"
+
+namespace {
+
+using browseros::windrise::LayoutMetric;
+
+constexpr char kPanelActionEventName[] = "windrise.onAiPanelAction";
+constexpr char kWindriseSettingsFallbackUrl[] =
+    "chrome://windrise/settings#connection";
+
+// 面板按钮发给扩展的 action 名（扩展 sw.js forwardGridAction 分发）。
+constexpr char kActionConnectAi[] = "connect_ai";
+constexpr char kActionOpenApp[] = "open_app";
+// 原来叫 "grid"，和工具栏格子按钮撞名；扩展两个名字都认（sw.js:1693）。
+constexpr char kActionGallery[] = "gallery";
+constexpr char kActionSettings[] = "settings";
+
+// 设计稿（ntp-zh-panel / ntp-en-panel，2x 图按 1/2 量）：
+// 「接一个 AI」按钮 36 高（偏好 ai_panel_button_height）、圆角 10、字 14
+// Medium，底 rgb(255 255 255 / .78)；底部文字行字 14，线到字顶 10；
+// 「闪办广场」下面的小字 11。
+// 字级（Opus-2 11-23，去「调试框感」）：标题 15 Medium（面板里唯一的标题），
+// 行里 AI 名 14 Regular、时间 12 次级色，底部文字行 14；以前标题取的是对话框
+// 标题字体，字号随系统走，和行字没有拉开层次。
+constexpr int kTitleSize = 15;
+constexpr int kTextSize = 14;
+constexpr int kTimeSize = 12;
+constexpr int kCaptionSize = 11;
+// 标题到第一行 / 到「接一个 AI」：已连接时 6（行自带 35 行高的上下留白），
+// 还没接时 12（标题下面直接是按钮，给一点呼吸）。
+constexpr int kTitleGapConnected = 6;
+constexpr int kTitleGapEmpty = 12;
+// 行里 AI 标志：16 见方、白底圆角 4（和左下卡片里的同一种小方块，乙05），
+// 到名字 8。
+constexpr int kRowIconSize = 16;
+constexpr int kRowIconRadius = 4;
+constexpr int kRowIconGap = 8;
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
+// 按下缩到 .97、120ms（emil-design-eng「Buttons must feel responsive」；
+// design-language.md 按钮三态「按下 scale(.97) 120ms」）。松开稍慢 160ms
+// （review-animations 第 9 条：按 / 松不对称）；曲线 WINDRISE_EASE_OUT =
+// cubic-bezier(.23,1,.32,1)，
+// 合成器逐帧推进（120Hz 屏 120 帧，已定 ）；中途松开从当前值反向，不跳。
+// 系统关了动画（Windows「显示动画」= 减少动态效果）时一帧到位，颜色反馈照旧。
+constexpr float kPressedScale = 0.97f;
+constexpr base::TimeDelta kPressDuration = base::Milliseconds(120);
+constexpr base::TimeDelta kReleaseDuration = base::Milliseconds(160);
+// 不可用：整颗按钮 45% 不透明（design-language.md 按钮三态「禁用 45% 不透明」）。
+constexpr float kDisabledOpacity = 0.45f;
+
+gfx::ImageSkia DecodeIcon(const std::string& png) {
+  if (png.empty()) {
+    return gfx::ImageSkia();
+  }
+  // 同 windrise_ai_card_view.cc：158 的 PNGCodec 只剩返回 SkBitmap 的重载；
+  // 尺寸已在扩展接口处按 IHDR 限过。
+  const SkBitmap bitmap = gfx::PNGCodec::Decode(base::as_byte_span(png));
+  if (bitmap.isNull()) {
+    return gfx::ImageSkia();
+  }
+  return gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
+}
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
+  label->SetFontList(SizedFont(label->font_list(), kTextSize, medium));
+  return label;
+}
+
+// 帧里「2 分钟前」「昨天 16:12」：一小时内写「N 分钟前」，今天写「今天 HH:MM」，
+// 昨天写「昨天 HH:MM」，更早写「M 月 D 日」（英文 Today 16:12 / Yesterday
+// 16:12 / Nov 14）。两张帧都是 24 小时制，中英文都按 24 小时写。扩展现在只推
+// 「距今分钟数」，绝对时间 = 现在 - 分钟数；今天 / 昨天按本地零点分。
+// 英文较早的日期写「月份缩写 + 日」（Nov 14），不写 11/14：美式读「11 月 14 日」、
+// 欧式读「11 日」会混。中文界面仍用「M 月 D 日」那条（字一个不变）。月份缩写
+// 是固定的英文三个字母：风桥自己的字符串只有中英两套，非中文界面一律是英文。
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
+  if (base::StartsWith(base::i18n::GetConfiguredLocale(), "zh")) {
+    return l10n_util::GetStringFUTF16(IDS_WINDRISE_AI_MONTH_DAY,
+                                      base::FormatNumber(exploded.month),
+                                      base::FormatNumber(exploded.day_of_month));
+  }
+  static constexpr std::array<std::u16string_view, 12> kMonthAbbr = {
+      u"Jan", u"Feb", u"Mar", u"Apr", u"May", u"Jun",
+      u"Jul", u"Aug", u"Sep", u"Oct", u"Nov", u"Dec"};
+  const size_t month_index =
+      static_cast<size_t>(std::clamp(exploded.month, 1, 12) - 1);
+  return l10n_util::GetStringFUTF16(IDS_WINDRISE_AI_MONTH_NAME_DAY,
+                                    std::u16string(kMonthAbbr[month_index]),
+                                    base::FormatNumber(exploded.day_of_month));
+}
+
+// 面板里的两种按钮：kFilled =「接一个 AI」白底宽按钮；kText = 底部文字行。
+// 反馈（emil-design-eng：可按的东西都要有按下反馈，不加别的动效）：悬停叠
+// 墨色 4%；按下叠 8% 并缩到 .97；键盘焦点走系统焦点环（形状同底的圆角）。
+// 不可用（闪办广场，完整版即将上线）时不画悬停 / 按下底、不缩放，整颗 45%。
+class PanelButton : public views::LabelButton {
+  METADATA_HEADER(PanelButton, views::LabelButton)
+
+ public:
+  enum class Style { kFilled, kText };
+
+  PanelButton(PressedCallback callback,
+              const std::u16string& text,
+              Style style,
+              bool muted = false)
+      : views::LabelButton(std::move(callback), text), style_(style) {
+    const bool filled = style_ == Style::kFilled;
+    label()->SetFontList(
+        SizedFont(label()->font_list(), kTextSize, /*medium=*/filled));
+    const ui::ColorId text_color = muted
+                                       ? kColorTabForegroundInactiveFrameActive
+                                       : kColorTabForegroundActiveFrameActive;
+    SetEnabledTextColors(ui::ColorVariant(text_color));
+    // 不可用时字色不换，整颗按钮降到 45%（见 StateChanged）。
+    SetTextColor(views::Button::STATE_DISABLED, ui::ColorVariant(text_color));
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
+    // 焦点环用墨色（Windrise 配色里 kColorSysStateFocusRing = 主文字色），不用
+    // 上游默认的蓝色强调色（已定 界面不出现彩色）。
+    if (views::FocusRing* ring = views::FocusRing::Get(this)) {
+      ring->SetColorId(ui::kColorSysStateFocusRing);
+    }
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
+    if (!layer()) {
+      return;  // 构造里建层之前的状态变化。
+    }
+    layer()->SetOpacity(GetState() == STATE_DISABLED ? kDisabledOpacity : 1.0f);
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
+  // B 版银色玻璃（已定 漏做核查 L7）：渐变、高光由 OnPaintBackground
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
+  // 「接一个 AI」：行下面的白底宽按钮（两张帧；方案 「36 高圆角 10、底
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
+  // 线到字顶 10：文字按钮自带上内边 4，所以这一行上边距 10 - 4；左右各伸出
+  // 按钮自带的内边 6，字和上面的内容在同一条竖线上。
+  auto* footer = AddChildView(std::make_unique<views::BoxLayoutView>());
+  footer->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
+  footer->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kStart);
+  footer->SetBetweenChildSpacing(kFooterTextGap - 2 * kTextButtonInsetH);
+  footer->SetProperty(
+      views::kMarginsKey,
+      gfx::Insets::TLBR(kSectionGap - kTextButtonInsetV, -kTextButtonInsetH, 0,
+                        -kTextButtonInsetH));
+
+  const auto make_text_button = [this](int string_id, const char* action,
+                                       bool muted = false) {
+    return std::make_unique<PanelButton>(
+        base::BindRepeating(&WindriseAiPanelView::NotifyAction,
+                            base::Unretained(this), action),
+        l10n_util::GetStringUTF16(string_id), PanelButton::Style::kText, muted);
+  };
+  footer->AddChildView(
+      make_text_button(IDS_WINDRISE_AI_PANEL_OPEN_APP, kActionOpenApp));
+
+  // 闪办广场（U039 口径：首发不上完整版，对外只说「完整版即将上线」）。
+  // 保持可点击且外观柔和，下面配一行小字「完整版即将上线」；点击时既通知
+  // 扩展（sw.js 按未上线安全拦下不跳转），又在浏览器内弹出温和清晰的 toast
+  // 提示「闪办广场 · 完整版即将上线」，避免禁用死按钮让用户点完毫无反应。
+  auto* gallery_column =
+      footer->AddChildView(std::make_unique<views::BoxLayoutView>());
+  gallery_column->SetOrientation(views::BoxLayout::Orientation::kVertical);
+  gallery_column->SetCrossAxisAlignment(
+      views::BoxLayout::CrossAxisAlignment::kStart);
+  const std::u16string coming_soon =
+      l10n_util::GetStringUTF16(IDS_WINDRISE_AI_GALLERY_SOON);
+  auto* gallery = gallery_column->AddChildView(
+      make_text_button(IDS_WINDRISE_AI_GALLERY, kActionGallery, /*muted=*/true));
+  gallery->GetViewAccessibility().SetDescription(coming_soon);
+  auto* caption = gallery_column->AddChildView(MakeLabel(
+      coming_soon, kColorTabForegroundInactiveFrameActive, /*medium=*/false));
+  caption->SetFontList(
+      SizedFont(caption->font_list(), kCaptionSize, /*medium=*/false));
+  // 小字和「闪办广场」左对齐，紧贴上一行（按钮自带下内边 4，这里收回 2）。
+  // 小字的宽会算进这一列的宽：英文用短的 Coming soon，中英文三项加小字都在
+  // 196 + 12 以内。
+  caption->SetProperty(views::kMarginsKey,
+                       gfx::Insets::TLBR(0, kTextButtonInsetH, 0, 0));
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
+  // 和卡片同一套银色玻璃（windrise_ai_card_view.cc OnPaintBackground；方案
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
+  views::Label* title_label = rows_container_->AddChildView(MakeLabel(
+      title, kColorTabForegroundActiveFrameActive, /*medium=*/true));
+  title_label->SetFontList(
+      SizedFont(title_label->font_list(), kTitleSize, /*medium=*/true));
+  title_label->SetProperty(
+      views::kMarginsKey,
+      gfx::Insets::TLBR(0, 0,
+                        count == 0 ? kTitleGapEmpty - kSectionGap
+                                   : kTitleGapConnected,
+                        0));
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
+    // 各家 AI 的真标志（扩展推来的 PNG，乙05）；没给就不占位，名字顶到左边。
+    const gfx::ImageSkia icon = DecodeIcon(connection.icon_png);
+    if (!icon.isNull()) {
+      auto* image = row->AddChildView(std::make_unique<views::ImageView>());
+      image->SetImage(ui::ImageModel::FromImageSkia(icon));
+      image->SetImageSize(gfx::Size(kRowIconSize, kRowIconSize));
+      image->SetBackground(views::CreateRoundedRectBackground(
+          ui::ColorVariant(SK_ColorWHITE), static_cast<float>(kRowIconRadius)));
+      image->SetProperty(views::kMarginsKey,
+                         gfx::Insets::TLBR(0, 0, 0, kRowIconGap));
+    }
+    views::Label* name = row->AddChildView(MakeLabel(
+        connection.name, kColorTabForegroundActiveFrameActive,
+        /*medium=*/false));
+    name->SetElideBehavior(gfx::ELIDE_TAIL);
+    // 名字占满中间（长名字省略号收尾，不把时间挤出面板）。
+    row->SetFlexForView(name, 1, /*use_min_size=*/true);
+    if (connection.last_active_minutes >= 0) {
+      views::Label* when = row->AddChildView(MakeLabel(
+          FormatLastActive(connection.last_active_minutes),
+          kColorTabForegroundInactiveFrameActive, /*medium=*/false));
+      when->SetFontList(SizedFont(when->font_list(), kTimeSize, false));
+      when->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 8, 0, 0));
+    }
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
+  BrowserWindowInterface* browser = browser_.get();
+  Profile* profile = browser ? browser->GetProfile() : nullptr;
+  if (!profile) {
+    return;
+  }
+  auto* registry = extensions::ExtensionRegistry::Get(profile);
+  const extensions::Extension* assistant =
+      registry ? registry->enabled_extensions().GetByID(
+                     browseros::kWindriseAssistantExtensionId)
+               : nullptr;
+  const bool desktop_connected =
+      assistant && extensions::IsAuthorizedWindriseExtension(assistant);
+
+  if (auto* event_router = extensions::EventRouter::Get(profile)) {
+    auto args = base::MakeRefCounted<extensions::EventArgs>();
+    args->data.Append(action);
+    event_router->BroadcastEvent(std::make_unique<extensions::Event>(
+        extensions::events::UNKNOWN, kPanelActionEventName, std::move(args),
+        profile));
+  }
+
+  const std::string_view action_view(action);
+  if (action_view == kActionGallery) {
+    if (ToastController* toast_controller = ToastController::From(browser)) {
+      ToastParams toast_params(ToastId::kBrowserOSToast);
+      toast_params.body_string_override =
+          l10n_util::GetStringUTF16(IDS_WINDRISE_AI_GALLERY) + u" · " +
+          l10n_util::GetStringUTF16(IDS_WINDRISE_AI_GALLERY_SOON);
+      toast_controller->MaybeShowToast(std::move(toast_params));
+    }
+  } else if (action_view == kActionSettings) {
+    // 兜底直接打开（或聚焦）风桥整页设置；扩展收到 onAiPanelAction 后也会
+    // 复用同一标签页，确保直启或受管模式下点击「设置」必定立即响应。
+    ShowSingletonTab(browser, GURL(assistant ? kWindriseSettingsFallbackUrl
+                                             : chrome::kChromeUISettingsURL));
+  } else if ((action_view == kActionOpenApp ||
+              action_view == kActionConnectAi) &&
+             !desktop_connected) {
+    // 独立启动或桌面端未在线时，扩展 forwardGridAction 没有宿主 binding；
+    // 改为打开连接设置页提示接入桌面端，避免点完毫无反应。
+    ShowSingletonTab(browser, GURL(assistant ? kWindriseSettingsFallbackUrl
+                                             : chrome::kChromeUISettingsURL));
+  }
+
+  if (views::Widget* widget = GetWidget()) {
+    widget->Close();
+  }
+}
+
+BEGIN_METADATA(WindriseAiPanelView)
+END_METADATA
\ No newline at end of file
