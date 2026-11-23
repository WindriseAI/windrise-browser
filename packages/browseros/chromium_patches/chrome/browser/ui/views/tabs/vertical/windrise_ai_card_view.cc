diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.cc b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.cc
new file mode 100644
index 0000000000..c015253555
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_card_view.cc
@@ -0,0 +1,717 @@
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
+#include "chrome/browser/ui/views/frame/browser_view.h"
+#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
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
+#include "ui/gfx/color_utils.h"
+#include "ui/gfx/geometry/skia_conversions.h"
+#include "ui/gfx/skia_paint_util.h"
+#include "ui/gfx/image/image_skia.h"
+#include "ui/events/event.h"
+#include "ui/views/background.h"
+#include "ui/views/controls/focus_ring.h"
+#include "ui/views/controls/highlight_path_generator.h"
+#include "ui/views/focus/focus_manager.h"
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
+// chrome.windrise.onAiPanelAction（见 windrise.idl）。
+constexpr char kPanelActionEventName[] = "windrise.onAiPanelAction";
+
+// 每一组内容淡入 / 淡出各 180ms（状态切换档），曲线 T0。
+constexpr base::TimeDelta kContentFadeDuration = base::Milliseconds(180);
+// 对齐 WindriseAiSectionView：在窗口内由合成器按屏幕刷新率驱动（120Hz 屏 120 帧），
+// 进窗口前退回 120Hz 节拍，不再写死 60Hz 默认帧率。
+constexpr int kContentFadeFrameRate = 120;
+// 旧组透明度降到这个值时，新组无论如何都开始淡入（兜底，保证两组有重叠）。
+// 不按时间兜底：T0 前段很陡，旧组在 1/3 时长时已经接近 0，按时间等会留空档
+// （补录 R1：展开 / 首次收起时卡片有约 33ms 近乎全空，见盘点 8.14）。
+constexpr float kFadeInStartOpacity = 0.5f;
+
+// 焦点环（Opus-2 11-23，根因分析 B7②：深 / 浅色下点完卡片外圈挂一圈亮白粗矩形）。
+// 形状 = 卡片自己的圆角（HighlightPathGenerator），1.5px 单层，颜色跟卡片文字
+// （浅色墨色、深色石墨白，甲48「清晰单层墨色圈」），只在键盘 Tab 走到卡片、或
+// 键盘打开面板后焦点还回卡片时显示；鼠标点开再关面板不显示。
+constexpr float kFocusRingThickness = 1.5f;
+
+// 三态的描边（Opus-2 11-23）。未连接 / 已连接：.5px 分隔线色；AI 正在操作：
+// 描边换成文字色 24%，像玻璃边缘被点亮一圈（不加新元素、不加动效——AI 操作时
+// 网页四周已经有流光，这里只是安静地标一下状态）。
+constexpr SkAlpha kWorkingRimAlpha = 0x3D;
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
+WindriseAiCardView::FadeDelegate::FadeDelegate(views::View* host,
+                                               WindriseAiCardView* owner)
+    : views::AnimationDelegateViews(host), owner_(owner) {}
+
+WindriseAiCardView::FadeDelegate::~FadeDelegate() = default;
+
+void WindriseAiCardView::FadeDelegate::AnimationProgressed(
+    const gfx::Animation* animation) {
+  owner_->AnimationProgressed(animation);
+}
+
+void WindriseAiCardView::FadeDelegate::AnimationEnded(
+    const gfx::Animation* animation) {
+  owner_->AnimationEnded(animation);
+}
+
+WindriseAiCardView::WindriseAiCardView(BrowserWindowInterface* browser)
+    : views::Button(base::BindRepeating(&WindriseAiCardView::ShowPanel,
+                                        base::Unretained(this))),
+      browser_(browser),
+      fade_out_delegate_(this, this),
+      fade_in_delegate_(this, this),
+      fade_out_animation_(
+          gfx::Animation::RichAnimationDuration(kContentFadeDuration),
+          kContentFadeFrameRate,
+          &fade_out_delegate_),
+      fade_in_animation_(
+          gfx::Animation::RichAnimationDuration(kContentFadeDuration),
+          kContentFadeFrameRate,
+          &fade_in_delegate_) {
+  SetAccessibleName(l10n_util::GetStringUTF16(IDS_WINDRISE_AI_NONE_CONNECTED));
+  SetFocusBehavior(views::View::FocusBehavior::ALWAYS);
+  InstallFocusRingStyle();
+  SetTooltipText(l10n_util::GetStringUTF16(IDS_WINDRISE_AI_OPEN_APP));
+  SetPaintToLayer();
+  layer()->SetFillsBoundsOpaquely(false);
+
+  // 折叠时「标志 + 数字」的标志：风桥钩形标志（设计稿 ntp-zh-rail 左下、theme-v2 §5、
+  // 标注 #6、已定 ；原来误用了闪办的闪电）。
+  mark_ = AddChildView(std::make_unique<views::ImageView>());
+  mark_->SetImage(ui::ImageModel::FromVectorIcon(
+      kWindriseBridgeMarkIcon, kColorTabForegroundActiveFrameActive,
+      browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kAiCardIconSize)));
+  mark_->SetVisible(false);
+
+  count_label_ = AddChildView(std::make_unique<views::Label>());
+  count_label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
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
+void WindriseAiCardView::InstallFocusRingStyle() {
+  views::InstallRoundRectHighlightPathGenerator(
+      this, gfx::Insets(),
+      browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kAiCardRadius));
+  views::FocusRing* ring = views::FocusRing::Get(this);
+  if (!ring) {
+    return;
+  }
+  ring->SetColorId(kColorTabForegroundActiveFrameActive);
+  ring->SetHaloThickness(kFocusRingThickness);
+  // 只认键盘带来的焦点（见 OnFocus）。
+  ring->SetHasFocusPredicate(base::BindRepeating([](const views::View* view) {
+    const auto* card = static_cast<const WindriseAiCardView*>(view);
+    return card->HasFocus() && card->keyboard_focus_;
+  }));
+}
+
+void WindriseAiCardView::OnFocus() {
+  const views::FocusManager* focus_manager = GetFocusManager();
+  const views::FocusManager::FocusChangeReason reason =
+      focus_manager ? focus_manager->focus_change_reason()
+                    : views::FocusManager::FocusChangeReason::kDirectFocusChange;
+  // Tab / Shift+Tab 走到这里：显示。面板关掉、焦点还回来：上一次是用键盘
+  // 打开的才显示。其它（程序直接给焦点）不显示。
+  keyboard_focus_ =
+      reason == views::FocusManager::FocusChangeReason::kFocusTraversal ||
+      (reason == views::FocusManager::FocusChangeReason::kFocusRestore &&
+       last_input_was_key_);
+  views::Button::OnFocus();
+  if (views::FocusRing* ring = views::FocusRing::Get(this)) {
+    ring->Refresh();
+  }
+}
+
+void WindriseAiCardView::OnBlur() {
+  keyboard_focus_ = false;
+  views::Button::OnBlur();
+  if (views::FocusRing* ring = views::FocusRing::Get(this)) {
+    ring->Refresh();
+  }
+}
+
+bool WindriseAiCardView::OnMousePressed(const ui::MouseEvent& event) {
+  last_input_was_key_ = false;
+  if (keyboard_focus_) {
+    // 键盘走到卡片后又用鼠标点：从这一下起按鼠标算，焦点环收起。
+    keyboard_focus_ = false;
+    if (views::FocusRing* ring = views::FocusRing::Get(this)) {
+      ring->Refresh();
+    }
+  }
+  return views::Button::OnMousePressed(event);
+}
+
+bool WindriseAiCardView::OnKeyPressed(const ui::KeyEvent& event) {
+  last_input_was_key_ = true;
+  return views::Button::OnKeyPressed(event);
+}
+
+void WindriseAiCardView::SetCollapsed(bool collapsed) {
+  if (collapsed_ == collapsed) {
+    return;
+  }
+  collapsed_ = collapsed;
+  const size_t entering = GroupIndex(collapsed_);
+  const size_t leaving = GroupIndex(!collapsed_);
+  if (!gfx::Animation::ShouldRenderRichAnimation() || !GetWidget()) {
+    // 减少动态效果 / 还没进窗口：直接换（原行为）。
+    fade_out_animation_.Stop();
+    fade_in_animation_.Stop();
+    fade_in_pending_ = false;
+    group_opacity_[entering] = 1.0f;
+    group_opacity_[leaving] = 0.0f;
+    ClearContentLayers();
+    RebuildFromState();
+    PreferredSizeChanged();
+    SchedulePaint();
+    return;
+  }
+  // LinearAnimation::Stop() 只发 AnimationCanceled（这里没覆写，什么都不做）。
+  fade_out_animation_.Stop();
+  fade_in_animation_.Stop();
+  // 两组都从各自当前的透明度起步（被打断时就是从正在显示的值接着走）。
+  group_from_[leaving] = group_opacity_[leaving];
+  group_from_[entering] = group_opacity_[entering];
+  fade_start_width_ = width();
+  // 两组都要在场：先排版、建层并设好透明度，再开始走。
+  fade_in_pending_ = true;
+  RebuildFromState();
+  ApplyGroupOpacity(false);
+  ApplyGroupOpacity(true);
+  // 旧的一组立刻淡出。Start() 不会清进度（第一帧 Step 前 GetCurrentValue()
+  // 还是上一次的值），先归零，免得时间兜底误判成「已过一半」。
+  fade_out_animation_.SetCurrentValue(0.0);
+  fade_out_animation_.Start();
+  // 新的一组：已经在屏幕上（来回切被打断）就直接接着淡入；否则等宽度过中点。
+  MaybeStartFadeIn(/*force=*/group_opacity_[entering] > 0.0f);
+}
+
+std::vector<views::View*> WindriseAiCardView::GroupViews(
+    bool collapsed_group) const {
+  std::vector<views::View*> views;
+  if (collapsed_group) {
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
+bool WindriseAiCardView::GroupShown(bool collapsed_group) const {
+  // 目标那一组总在；另一组只在交叉淡还没淡完时在。
+  return collapsed_group == collapsed_ ||
+         group_opacity_[GroupIndex(collapsed_group)] > 0.0f;
+}
+
+void WindriseAiCardView::ApplyGroupOpacity(bool collapsed_group) {
+  const float opacity = group_opacity_[GroupIndex(collapsed_group)];
+  for (views::View* view : GroupViews(collapsed_group)) {
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
+  for (bool collapsed_group : {false, true}) {
+    for (views::View* view : GroupViews(collapsed_group)) {
+      if (view->layer()) {
+        view->DestroyLayer();
+      }
+    }
+  }
+}
+
+bool WindriseAiCardView::FadeInGateReached() const {
+  const int target = settled_width_[GroupIndex(collapsed_)];
+  if (target <= 0) {
+    return false;  // 不知道目标宽度：交给透明度兜底。
+  }
+  if (target == fade_start_width_) {
+    // 卡片在切换开始时已经是目标宽度（展开时侧栏先一步把卡片排到全宽）：
+    // 没有「过中点」可等，直接开始。
+    return true;
+  }
+  const int mid = (fade_start_width_ + target) / 2;
+  return target > fade_start_width_ ? width() >= mid : width() <= mid;
+}
+
+void WindriseAiCardView::MaybeStartFadeIn(bool force) {
+  if (!fade_in_pending_) {
+    return;
+  }
+  const bool old_faded =
+      !fade_out_animation_.is_animating() ||
+      group_opacity_[GroupIndex(!collapsed_)] <= kFadeInStartOpacity;
+  if (!force && !old_faded && !FadeInGateReached()) {
+    return;
+  }
+  fade_in_pending_ = false;
+  fade_in_animation_.SetCurrentValue(0.0);
+  fade_in_animation_.Start();
+}
+
+void WindriseAiCardView::FinishFadeIfDone() {
+  if (fade_in_pending_ || fade_out_animation_.is_animating() ||
+      fade_in_animation_.is_animating()) {
+    return;
+  }
+  group_opacity_[GroupIndex(collapsed_)] = 1.0f;
+  group_opacity_[GroupIndex(!collapsed_)] = 0.0f;
+  ClearContentLayers();
+  RebuildFromState();
+  SchedulePaint();
+}
+
+void WindriseAiCardView::AnimationProgressed(const gfx::Animation* animation) {
+  if (animation != &fade_out_animation_ && animation != &fade_in_animation_) {
+    views::Button::AnimationProgressed(animation);
+    return;
+  }
+  const float e = static_cast<float>(gfx::Tween::CalculateValue(
+      gfx::Tween::WINDRISE_EASE_OUT, animation->GetCurrentValue()));
+  if (animation == &fade_out_animation_) {
+    const size_t leaving = GroupIndex(!collapsed_);
+    group_opacity_[leaving] = group_from_[leaving] * (1.0f - e);
+    ApplyGroupOpacity(!collapsed_);
+    MaybeStartFadeIn(/*force=*/false);
+  } else {
+    const size_t entering = GroupIndex(collapsed_);
+    group_opacity_[entering] =
+        group_from_[entering] + (1.0f - group_from_[entering]) * e;
+    ApplyGroupOpacity(collapsed_);
+  }
+}
+
+void WindriseAiCardView::AnimationEnded(const gfx::Animation* animation) {
+  if (animation != &fade_out_animation_ && animation != &fade_in_animation_) {
+    views::Button::AnimationEnded(animation);
+    return;
+  }
+  if (animation == &fade_out_animation_) {
+    // 旧组淡完时新组还在等：不再等，立刻开始（兜底；正常早就开始了）。
+    MaybeStartFadeIn(/*force=*/true);
+  }
+  FinishFadeIfDone();
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
+  working_ = ai_state && !ai_state->active_tab_ids.empty();
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
+  const bool show_collapsed = GroupShown(true);
+  const bool show_expanded = GroupShown(false);
+  mark_->SetVisible(show_collapsed);
+  count_label_->SetVisible(show_collapsed);
+  text_->SetVisible(show_expanded);
+  gear_->SetVisible(show_expanded);
+
+  const int icon_size =
+      browseros::windrise::GetLayoutMetrics().Get(LayoutMetric::kAiCardIconSize);
+  for (size_t i = 0; i < ai_icons_.size(); ++i) {
+    views::ImageView* icon = ai_icons_[i];
+    const bool visible = show_expanded && ai_state && i < count &&
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
+  // 未连接时「还没接 AI」用次级文字色（层次退一档：这是提示，不是状态值）；
+  // 已连接 / AI 正在操作用主文字色。
+  text_->SetEnabledColor(ui::ColorVariant(
+      count == 0 ? kColorTabForegroundInactiveFrameActive
+                 : kColorTabForegroundActiveFrameActive));
+  count_label_->SetEnabledColor(
+      ui::ColorVariant(kColorTabForegroundActiveFrameActive));
+  // theme-v2 §5：折叠时「标志 + 数字」和标签的「图标 + 一个字」同构，数字用
+  // 同一个字号（偏好 rail_glyph_size，默认 14）。
+  const int glyph_size = browseros::windrise::GetLayoutMetrics().Get(
+      LayoutMetric::kRailGlyphSize);
+  if (count_label_->font_list().GetFontSize() != glyph_size) {
+    count_label_->SetFontList(count_label_->font_list().DeriveWithSizeDelta(
+        glyph_size - count_label_->font_list().GetFontSize()));
+  }
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
+  const int center_y = height() / 2;
+  // 交叉淡期间两组同时在场，各排各的（互不挤位置）。
+  if (GroupShown(true)) {
+    LayoutCollapsedGroup(center_y);
+  } else {
+    mark_->SetBounds(0, 0, 0, 0);
+    count_label_->SetBounds(0, 0, 0, 0);
+  }
+  if (GroupShown(false)) {
+    LayoutExpandedGroup(center_y);
+  } else {
+    text_->SetBounds(0, 0, 0, 0);
+    gear_->SetBounds(0, 0, 0, 0);
+    for (views::ImageView* icon : ai_icons_) {
+      icon->SetBounds(0, 0, 0, 0);
+    }
+  }
+  // 稳定时记下这种状态的宽度，下次切换用来判「宽度过中点」；
+  // 交叉淡进行中（侧栏宽度每帧在变、每帧都会排版）看一眼要不要开始淡入新组。
+  if (!fade_out_animation_.is_animating() &&
+      !fade_in_animation_.is_animating() && !fade_in_pending_) {
+    settled_width_[GroupIndex(collapsed_)] = width();
+  } else {
+    MaybeStartFadeIn(/*force=*/false);
+  }
+}
+
+void WindriseAiCardView::LayoutCollapsedGroup(int center_y) {
+  // theme-v2 §5 / 已定 折叠时「标志 + 数字」和标签的「图标 + 一个字」
+  // 同构——同一组尺寸（标志 16、间距 5、数字占 14 宽的格子、字号 14），同样
+  // 作为一组在卡片宽里居中，数字同样按字形对齐标志中线
+  // （TabViewVerticalLayout 里那一段的同一套算法）。卡片和标签一样宽、一样
+  // 居中，所以标志和上面每个标签的图标在同一条竖线上。数字到两位（10 起）
+  // 比格子宽时格子跟着放宽，一组仍居中。
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const int icon_size = metrics.Get(LayoutMetric::kAiCardIconSize);
+  const int gap = metrics.Get(LayoutMetric::kRailGlyphGap);
+  const gfx::FontList& font = count_label_->font_list();
+  const int text_width = count_label_->GetPreferredSize().width();
+  const int slot =
+      std::max(metrics.Get(LayoutMetric::kRailGlyphSize), text_width);
+  const int group_width = icon_size + gap + slot;
+  // 按收起到位时的卡片宽居中（交叉淡期间卡片宽度还在变，这一组不跟着横移，
+  // 和标签图标一样始终在同一个 x）。
+  const int base_width = collapsed_content_width_ > 0
+                             ? std::min(width(), collapsed_content_width_)
+                             : width();
+  const int group_x = std::max(0, (base_width - group_width + 1) / 2);
+  const int icon_y = center_y - icon_size / 2;
+  mark_->SetBounds(group_x, icon_y, icon_size, icon_size);
+  // 行高等于字体高时基线就在 GetBaseline()；字形中线 = 基线 - 大写高 / 2，
+  // 对齐标志中线。左右各留 4，数字不被自己的边界裁掉。
+  constexpr int kGlyphOverhang = 4;
+  const int mark_center_y = icon_y + icon_size / 2;
+  const int baseline_y = mark_center_y + (font.GetCapHeight() + 1) / 2;
+  const int slot_center_x = group_x + icon_size + gap + slot / 2;
+  const int label_width = slot + 2 * kGlyphOverhang;
+  count_label_->SetBounds(slot_center_x - label_width / 2,
+                          baseline_y - font.GetBaseline(), label_width,
+                          font.GetHeight());
+}
+
+void WindriseAiCardView::LayoutExpandedGroup(int center_y) {
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const int icon_size = metrics.Get(LayoutMetric::kAiCardIconSize);
+  const int gear_size = metrics.Get(LayoutMetric::kTopButtonIconSize);
+  const int right = 12;
+  int x = metrics.Get(LayoutMetric::kAiCardPaddingLeft);
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
+  text_->SetBounds(x, center_y - text_height / 2,
+                   std::max(0, width() - x - right - gear_size - 6),
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
+  // 暗色（底是深色两档）时高光收到和面板一样弱（.15），亮色照旧 .9。
+  const bool dark = color_utils::IsDark(
+      GetColorProvider()->GetColor(kColorWindriseAiCardBackgroundBottom));
+  highlight.setColor(dark ? SkColorSetA(SK_ColorWHITE, 0x26)
+                          : SkColorSetARGB(0xE6, 0xFF, 0xFF, 0xFF));
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
+  // .5px 边线（同样走偏好）。AI 正在操作时换成文字色 24% 的亮边（见 kWorkingRimAlpha）。
+  cc::PaintFlags hairline = flags;
+  hairline.setStyle(cc::PaintFlags::kStroke_Style);
+  hairline.setStrokeWidth(1.0f);
+  hairline.setColor(
+      working_
+          ? SkColorSetA(GetColorProvider()->GetColor(
+                            kColorTabForegroundActiveFrameActive),
+                        kWorkingRimAlpha)
+          : GetColorProvider()->GetColor(kColorWindriseDivider));
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
+  // 圆角走偏好，换主题时一起刷新焦点环的形状。
+  InstallFocusRingStyle();
+  mark_->SetImage(ui::ImageModel::FromVectorIcon(
+      kWindriseBridgeMarkIcon, kColorTabForegroundActiveFrameActive,
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
+  // 先锁住悬停展开的当前状态，再弹面板（面板关掉时在 OnWidgetDestroyed 里放开）。
+  if (BrowserView* browser_view =
+          BrowserView::GetBrowserViewForBrowser(browser_.get())) {
+    if (browser_view->tab_strip_view()) {
+      expand_on_hover_lock_ =
+          browser_view->tab_strip_view()->GetExpandOnHoverLock(
+              ExpandOnHoverLockType::kKeepCurrentState);
+    }
+  }
+  auto* panel = new WindriseAiPanelView(browser_.get());
+  panel_ = panel;
+  SchedulePaint();
+  panel->ShowAtAnchor(this, collapsed_);
+  if (panel->GetWidget()) {
+    panel_observation_.Observe(panel->GetWidget());
+  } else {
+    expand_on_hover_lock_.reset();  // 面板没建出来：不留着锁。
+  }
+}
+
+void WindriseAiCardView::OnWidgetDestroyed(views::Widget* widget) {
+  panel_observation_.Reset();
+  panel_ = nullptr;
+  expand_on_hover_lock_.reset();
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
