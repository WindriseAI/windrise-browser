diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.cc b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.cc
new file mode 100644
index 0000000000..7b6f53417c
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.cc
@@ -0,0 +1,480 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.h"
+
+#include <algorithm>
+#include <memory>
+#include <utility>
+
+#include "base/functional/bind.h"
+#include "base/containers/span.h"
+#include "base/i18n/number_formatting.h"
+#include "base/strings/utf_string_conversions.h"
+#include "cc/paint/paint_flags.h"
+#include "chrome/app/vector_icons/vector_icons.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/profiles/profile.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
+#include "chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.h"
+#include "chrome/grit/generated_resources.h"
+#include "extensions/browser/event_router.h"
+#include "extensions/browser/extension_event_histogram_value.h"
+#include "third_party/skia/include/core/SkBitmap.h"
+#include "third_party/skia/include/core/SkRRect.h"
+#include "ui/base/l10n/l10n_util.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/color/color_variant.h"
+#include "ui/compositor/layer.h"
+#include "ui/gfx/animation/animation.h"
+#include "ui/gfx/animation/tween.h"
+#include "ui/gfx/canvas.h"
+#include "ui/gfx/codec/png_codec.h"
+#include "ui/gfx/geometry/skia_conversions.h"
+#include "ui/gfx/skia_paint_util.h"
+#include "ui/gfx/image/image_skia.h"
+#include "ui/views/background.h"
+#include "ui/views/widget/widget.h"
+
+namespace {
+
+using browseros::windrise::LayoutMetric;
+
+// 卡片最多同时画几个 AI 标志（方案第 6 行：标志间距 2、之后到文字 10）。
+constexpr size_t kMaxShownAiIcons = 3;
+
+// 面板底部按钮点了以后发给扩展的事件名，扩展侧监听
+// chrome.browserOS.onAiPanelAction（见 browser_os.idl）。
+constexpr char kPanelActionEventName[] = "browserOS.onAiPanelAction";
+
+gfx::ImageSkia DecodeIcon(const std::string& png) {
+  if (png.empty()) {
+    return gfx::ImageSkia();
+  }
+  // 158 的 PNGCodec 只剩「返回 SkBitmap」这一个重载；解码失败返回空位图。
+  // 尺寸已在扩展接口处按 IHDR 限过（见 browser_os_api.cc），这里不再重复。
+  const SkBitmap bitmap = gfx::PNGCodec::Decode(base::as_byte_span(png));
+  if (bitmap.isNull()) {
+    return gfx::ImageSkia();
+  }
+  return gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
+}
+
+}  // namespace
+
+WindriseAiCardView::WindriseAiCardView(BrowserWindowInterface* browser)
+    : views::Button(base::BindRepeating(&WindriseAiCardView::ShowPanel,
+                                        base::Unretained(this))),
+      browser_(browser),
+      fade_animation_(base::Milliseconds(90),
+                      gfx::LinearAnimation::kDefaultFrameRate,
+                      this) {
+  SetAccessibleName(l10n_util::GetStringUTF16(IDS_WINDRISE_AI_NONE_CONNECTED));
+  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
+  SetTooltipText(l10n_util::GetStringUTF16(IDS_WINDRISE_AI_OPEN_APP));
+  SetPaintToLayer();
+  layer()->SetFillsBoundsOpaquely(false);
+
+  mark_ = AddChildView(std::make_unique<views::ImageView>());
+  mark_->SetImage(ui::ImageModel::FromVectorIcon(
+      kWindriseFlashIcon, kColorTabForegroundActiveFrameActive,
+      browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kAiCardIconSize)));
+  mark_->SetVisible(false);
+
+  count_label_ = AddChildView(std::make_unique<views::Label>());
+  count_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
+  count_label_->SetVisible(false);
+
+  for (size_t i = 0; i < kMaxShownAiIcons; ++i) {
+    auto icon = std::make_unique<views::ImageView>();
+    icon->SetImageSize(gfx::Size(
+        browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kAiCardIconSize),
+        browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kAiCardIconSize)));
+    icon->SetVisible(false);
+    ai_icons_.push_back(AddChildView(std::move(icon)));
+  }
+
+  text_ = AddChildView(std::make_unique<views::Label>());
+  text_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
+
+  gear_ = AddChildView(std::make_unique<views::ImageView>());
+  gear_->SetImage(ui::ImageModel::FromVectorIcon(
+      kSettingsIcon, kColorTabForegroundInactiveFrameActive,
+      browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kTopButtonIconSize)));
+
+  if (browseros::windrise::AiWindowState* state =
+          browseros::windrise::AiWindowState::Attach(browser_.get())) {
+    state_subscription_ = state->AddObserver(base::BindRepeating(
+        &WindriseAiCardView::OnAiStateChanged, base::Unretained(this)));
+  }
+  OnAiStateChanged();
+}
+
+WindriseAiCardView::~WindriseAiCardView() {
+  state_subscription_ = {};
+  browseros::windrise::AiWindowState::Detach(browser_.get());
+}
+
+void WindriseAiCardView::SetCollapsed(bool collapsed) {
+  if (collapsed_ == collapsed) {
+    return;
+  }
+  collapsed_ = collapsed;
+  if (!gfx::Animation::ShouldRenderRichAnimation() || !GetWidget()) {
+    // 减少动态效果 / 还没进窗口：直接换（原行为）。
+    fade_animation_.Stop();
+    fade_phase_ = FadePhase::kIdle;
+    ClearContentLayers();
+    SwapShownContent();
+    return;
+  }
+  switch (fade_phase_) {
+    case FadePhase::kIdle:
+      StartFadePhase(FadePhase::kOut, 1.0f);
+      break;
+    case FadePhase::kOut:
+      // 还在淡出：接着淡，换内容那一刻按最新的 collapsed_ 换（来回点了偶数次
+      // 就是原内容淡出再淡回）。
+      break;
+    case FadePhase::kIn:
+      // 新内容淡入到一半目标又变了：从它当前的透明度接着淡出，不跳。
+      StartFadePhase(FadePhase::kOut, content_opacity_);
+      break;
+  }
+}
+
+std::vector<views::View*> WindriseAiCardView::ContentViews() const {
+  std::vector<views::View*> views;
+  if (shown_collapsed_) {
+    views = {mark_.get(), count_label_.get()};
+  } else {
+    views = {text_.get(), gear_.get()};
+    for (views::ImageView* icon : ai_icons_) {
+      views.push_back(icon);
+    }
+  }
+  return views;
+}
+
+void WindriseAiCardView::SetContentOpacity(float opacity) {
+  content_opacity_ = opacity;
+  for (views::View* view : ContentViews()) {
+    if (!view->GetVisible()) {
+      continue;
+    }
+    if (!view->layer()) {
+      // 只在交叉淡期间给子视图单独一层（结束就拆掉，平时照旧画在卡片层里）。
+      view->SetPaintToLayer();
+      view->layer()->SetFillsBoundsOpaquely(false);
+    }
+    view->layer()->SetOpacity(opacity);
+  }
+}
+
+void WindriseAiCardView::ClearContentLayers() {
+  content_opacity_ = 1.0f;
+  std::vector<views::View*> all = {mark_.get(), count_label_.get(),
+                                   text_.get(), gear_.get()};
+  for (views::ImageView* icon : ai_icons_) {
+    all.push_back(icon);
+  }
+  for (views::View* view : all) {
+    if (view->layer()) {
+      view->DestroyLayer();
+    }
+  }
+}
+
+void WindriseAiCardView::StartFadePhase(FadePhase phase, float from_opacity) {
+  fade_phase_ = phase;
+  fade_from_ = from_opacity;
+  // LinearAnimation::Stop() 只会发 AnimationCanceled（这里没覆写，什么都不做）。
+  fade_animation_.Stop();
+  SetContentOpacity(from_opacity);
+  fade_animation_.Start();
+}
+
+void WindriseAiCardView::SwapShownContent() {
+  shown_collapsed_ = collapsed_;
+  RebuildFromState();
+  PreferredSizeChanged();
+  SchedulePaint();
+}
+
+void WindriseAiCardView::AnimationProgressed(const gfx::Animation* animation) {
+  if (animation != &fade_animation_) {
+    views::Button::AnimationProgressed(animation);
+    return;
+  }
+  if (fade_phase_ == FadePhase::kIdle) {
+    return;
+  }
+  const float e = static_cast<float>(gfx::Tween::CalculateValue(
+      gfx::Tween::WINDRISE_EASE_OUT, animation->GetCurrentValue()));
+  SetContentOpacity(fade_phase_ == FadePhase::kOut
+                        ? fade_from_ * (1.0f - e)
+                        : fade_from_ + (1.0f - fade_from_) * e);
+}
+
+void WindriseAiCardView::AnimationEnded(const gfx::Animation* animation) {
+  if (animation != &fade_animation_) {
+    views::Button::AnimationEnded(animation);
+    return;
+  }
+  if (fade_phase_ == FadePhase::kOut) {
+    ClearContentLayers();
+    SwapShownContent();
+    StartFadePhase(FadePhase::kIn, 0.0f);
+    return;
+  }
+  fade_phase_ = FadePhase::kIdle;
+  ClearContentLayers();
+}
+
+void WindriseAiCardView::OnAiStateChanged() {
+  // AI 图标只在这里解码（RebuildFromState 在交叉淡换内容时也会调，不再解 PNG）。
+  browseros::windrise::AiWindowState* window_state =
+      browseros::windrise::AiWindowState::From(browser_.get());
+  decoded_icons_.clear();
+  if (window_state) {
+    const auto& connections = window_state->state().connections;
+    for (size_t i = 0; i < connections.size() && i < kMaxShownAiIcons; ++i) {
+      decoded_icons_.push_back(DecodeIcon(connections[i].icon_png));
+    }
+  }
+  RebuildFromState();
+  PreferredSizeChanged();
+  SchedulePaint();
+}
+
+void WindriseAiCardView::RebuildFromState() {
+  browseros::windrise::AiWindowState* state =
+      browseros::windrise::AiWindowState::From(browser_.get());
+  const browseros::windrise::AiState* ai_state =
+      state ? &state->state() : nullptr;
+  const size_t count = ai_state ? ai_state->connections.size() : 0;
+
+  if (count == 0) {
+    SetAccessibleName(l10n_util::GetStringUTF16(IDS_WINDRISE_AI_NONE_CONNECTED));
+    text_->SetText(l10n_util::GetStringUTF16(IDS_WINDRISE_AI_NONE_CONNECTED));
+  } else {
+    SetAccessibleName(l10n_util::GetStringFUTF16(
+        IDS_WINDRISE_AI_CONNECTED_COUNT,
+        base::FormatNumber(static_cast<int>(count))));
+    text_->SetText(l10n_util::GetStringFUTF16(
+        IDS_WINDRISE_AI_CONNECTED_COUNT,
+        base::FormatNumber(static_cast<int>(count))));
+  }
+  count_label_->SetText(base::FormatNumber(static_cast<int>(count)));
+
+  mark_->SetVisible(shown_collapsed_);
+  count_label_->SetVisible(shown_collapsed_);
+  text_->SetVisible(!shown_collapsed_);
+  gear_->SetVisible(!shown_collapsed_);
+
+  const int icon_size =
+      browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kAiCardIconSize);
+  for (size_t i = 0; i < ai_icons_.size(); ++i) {
+    views::ImageView* icon = ai_icons_[i];
+    const bool visible = !shown_collapsed_ && ai_state && i < count &&
+                         !ai_state->connections[i].icon_png.empty();
+    icon->SetVisible(visible);
+    if (!visible) {
+      continue;
+    }
+    icon->SetImage(ui::ImageModel::FromImageSkia(
+        i < decoded_icons_.size() ? decoded_icons_[i] : gfx::ImageSkia()));
+    icon->SetImageSize(gfx::Size(icon_size, icon_size));
+    // 16 见方白底圆角 4 的小方块（方案第 6 行）。
+    icon->SetBackground(views::CreateRoundedRectBackground(
+        ui::ColorVariant(SK_ColorWHITE), 4.0f));
+  }
+
+  text_->SetEnabledColor(ui::ColorVariant(kColorTabForegroundActiveFrameActive));
+  count_label_->SetEnabledColor(
+      ui::ColorVariant(kColorTabForegroundActiveFrameActive));
+  InvalidateLayout();
+}
+
+gfx::Size WindriseAiCardView::CalculatePreferredSize(
+    const views::SizeBounds& available_size) const {
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const int height = metrics.Get(LayoutMetric::kAiCardHeight);
+  const int width = available_size.width().value_or(
+      metrics.Get(LayoutMetric::kAiPanelWidth));
+  return gfx::Size(width, height);
+}
+
+void WindriseAiCardView::Layout(PassKey pass_key) {
+  LayoutSuperclass<views::Button>(this);
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const int icon_size = metrics.Get(LayoutMetric::kAiCardIconSize);
+  const int gear_size = metrics.Get(LayoutMetric::kTopButtonIconSize);
+  const int left = shown_collapsed_ ? 8 : metrics.Get(LayoutMetric::kAiCardPaddingLeft);
+  const int right = 12;
+  const int center_y = height() / 2;
+
+  int x = left;
+  if (shown_collapsed_) {
+    mark_->SetBounds(x, center_y - icon_size / 2, icon_size, icon_size);
+    x += icon_size + 2;
+    count_label_->SetBounds(
+        x, center_y - count_label_->GetPreferredSize().height() / 2,
+        width() - x - right, count_label_->GetPreferredSize().height());
+    // 收起时不画标志与文字。
+    text_->SetBounds(0, 0, 0, 0);
+    gear_->SetBounds(0, 0, 0, 0);
+    for (views::ImageView* icon : ai_icons_) {
+      icon->SetBounds(0, 0, 0, 0);
+    }
+    return;
+  }
+
+  mark_->SetBounds(0, 0, 0, 0);
+  count_label_->SetBounds(0, 0, 0, 0);
+
+  size_t shown = 0;
+  for (views::ImageView* icon : ai_icons_) {
+    if (!icon->GetVisible()) {
+      continue;
+    }
+    icon->SetBounds(x, center_y - icon_size / 2, icon_size, icon_size);
+    x += icon_size + 2;
+    ++shown;
+  }
+  if (shown > 0) {
+    x += 8;  // 标志与文字之间再留 8（方案第 6 行：2 → 10）。
+  }
+  const int text_height = text_->GetPreferredSize().height();
+  text_->SetBounds(x, center_y - text_height / 2, std::max(0, width() - x - right - gear_size - 6),
+                   text_height);
+  gear_->SetBounds(width() - right - gear_size, center_y - gear_size / 2,
+                   gear_size, gear_size);
+}
+
+void WindriseAiCardView::OnPaintBackground(gfx::Canvas* canvas) {
+  if (!GetColorProvider()) {
+    return;
+  }
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const float radius =
+      static_cast<float>(metrics.Get(LayoutMetric::kAiCardRadius));
+
+  cc::PaintFlags flags;
+  flags.setAntiAlias(true);
+  flags.setStyle(cc::PaintFlags::kFill_Style);
+  // B 版银色玻璃：上浅下深的竖向渐变（#F1F3F5 -> #E2E6EA，色值走偏好
+  // browseros.windrise.theme.*.ai_card_background_top / _bottom）+ 上缘 1px 白
+  // 高光 + .5px 线。渐变用 gfx::CreateGradientShader，圆角由 drawRRect 裁。
+  flags.setColor(
+      GetColorProvider()->GetColor(kColorWindriseAiCardBackground));
+  flags.setShader(gfx::CreateGradientShader(
+      gfx::Point(0, 0), gfx::Point(0, height()),
+      GetColorProvider()->GetColor(kColorWindriseAiCardBackgroundTop),
+      GetColorProvider()->GetColor(kColorWindriseAiCardBackgroundBottom)));
+  canvas->sk_canvas()->drawRRect(
+      SkRRect::MakeRectXY(gfx::RectToSkRect(GetLocalBounds()), radius, radius),
+      flags);
+  flags.setShader(nullptr);
+
+  // 上缘 1px 白高光：只画在圆角直边那一段，内缩圆角半径。
+  cc::PaintFlags highlight;
+  highlight.setAntiAlias(true);
+  highlight.setStyle(cc::PaintFlags::kStroke_Style);
+  highlight.setStrokeWidth(1.0f);
+  highlight.setColor(SkColorSetARGB(0xE6, 0xFF, 0xFF, 0xFF));
+  canvas->sk_canvas()->drawLine(radius, 0.5f, width() - radius, 0.5f,
+                                highlight);
+
+  // 方案标注表第 6 行：悬停在底色上叠墨色 4%；面板开着时底换成窗框色（#DDE1E5）。
+  if (panel_) {
+    cc::PaintFlags open = flags;
+    open.setColor(
+        GetColorProvider()->GetColor(kColorWindriseTabInactiveBackground));
+    canvas->sk_canvas()->drawRRect(
+        SkRRect::MakeRectXY(gfx::RectToSkRect(GetLocalBounds()), radius,
+                            radius),
+        open);
+  } else if (GetState() == views::Button::STATE_HOVERED ||
+             GetState() == views::Button::STATE_PRESSED) {
+    cc::PaintFlags hover = flags;
+    hover.setColor(SkColorSetA(
+        GetColorProvider()->GetColor(kColorTabForegroundActiveFrameActive),
+        GetState() == views::Button::STATE_PRESSED ? 20 : 10));
+    canvas->sk_canvas()->drawRRect(
+        SkRRect::MakeRectXY(gfx::RectToSkRect(GetLocalBounds()), radius,
+                            radius),
+        hover);
+  }
+
+  // .5px 边线（同样走偏好）。
+  cc::PaintFlags hairline = flags;
+  hairline.setStyle(cc::PaintFlags::kStroke_Style);
+  hairline.setStrokeWidth(1.0f);
+  hairline.setColor(GetColorProvider()->GetColor(kColorWindriseDivider));
+  const SkRect inset = gfx::RectToSkRect(GetLocalBounds());
+  canvas->sk_canvas()->drawRRect(
+      SkRRect::MakeRectXY(inset.makeInset(0.5f, 0.5f), radius, radius),
+      hairline);
+}
+
+void WindriseAiCardView::StateChanged(ButtonState old_state) {
+  views::Button::StateChanged(old_state);
+  SchedulePaint();
+}
+
+void WindriseAiCardView::OnThemeChanged() {
+  views::Button::OnThemeChanged();
+  mark_->SetImage(ui::ImageModel::FromVectorIcon(
+      kWindriseFlashIcon, kColorTabForegroundActiveFrameActive,
+      browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kAiCardIconSize)));
+  gear_->SetImage(ui::ImageModel::FromVectorIcon(
+      kSettingsIcon, kColorTabForegroundInactiveFrameActive,
+      browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kTopButtonIconSize)));
+  InvalidateLayout();
+}
+
+void WindriseAiCardView::ShowPanel() {
+  if (panel_) {
+    if (views::Widget* widget = panel_->GetWidget()) {
+      widget->Close();
+    }
+    return;
+  }
+  auto* panel = new WindriseAiPanelView(browser_.get());
+  panel_ = panel;
+  SchedulePaint();
+  panel->ShowAtAnchor(this, collapsed_);
+  if (panel->GetWidget()) {
+    panel_observation_.Observe(panel->GetWidget());
+  }
+}
+
+void WindriseAiCardView::OnWidgetDestroyed(views::Widget* widget) {
+  panel_observation_.Reset();
+  panel_ = nullptr;
+  SchedulePaint();
+}
+
+void WindriseAiCardView::NotifyPanelAction(const char* action) {
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
+BEGIN_METADATA(WindriseAiCardView)
+END_METADATA
\ No newline at end of file
