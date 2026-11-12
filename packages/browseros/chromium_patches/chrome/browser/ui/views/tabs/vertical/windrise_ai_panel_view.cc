diff --git a/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc
new file mode 100644
index 0000000..1650580
--- /dev/null
+++ b/chrome/browser/ui/views/tabs/vertical/windrise_ai_panel_view.cc
@@ -0,0 +1,220 @@
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
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/profiles/profile.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
+#include "chrome/grit/generated_resources.h"
+#include "extensions/browser/event_router.h"
+#include "extensions/browser/extension_event_histogram_value.h"
+#include "ui/base/l10n/l10n_util.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/base/mojom/dialog_button.mojom.h"
+#include "ui/color/color_variant.h"
+#include "ui/views/background.h"
+#include "ui/views/bubble/bubble_border.h"
+#include "ui/views/controls/button/label_button.h"
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
+// 面板底部三个按钮发给扩展的 action 名。
+constexpr char kActionOpenApp[] = "open_app";
+constexpr char kActionGallery[] = "grid";
+constexpr char kActionSettings[] = "settings";
+
+int Metric(LayoutMetric metric) {
+  return browseros::windrise::GetLayoutMetrics().Get(metric);
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
+  SetBackgroundColor(
+      ui::ColorVariant(kColorWindriseAiPanelBackground));
+  set_margins(gfx::Insets(Metric(LayoutMetric::kAiPanelPadding)));
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
+  auto* actions = AddChildView(std::make_unique<views::BoxLayoutView>());
+  actions->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
+  actions->SetBetweenChildSpacing(14);
+  // BoxLayoutView 没有 SetDefault；行上方的间距用子视图 margin 给
+  // （BoxLayout 与 FlexLayout 都认 views::kMarginsKey）。
+  actions->SetProperty(
+      views::kMarginsKey,
+      gfx::Insets::TLBR(Metric(LayoutMetric::kAiPanelPadding), 0, 0, 0));
+
+  const int button_height = Metric(LayoutMetric::kAiPanelButtonHeight);
+  const float radius = button_height / 2.0f;
+  const auto make_button = [&](int string_id,
+                               const char* action) -> views::LabelButton* {
+    auto button =
+        std::make_unique<views::LabelButton>(base::BindRepeating(
+            &WindriseAiPanelView::NotifyAction, base::Unretained(this),
+            action));
+    button->SetText(l10n_util::GetStringUTF16(string_id));
+    button->SetMinSize(gfx::Size(0, button_height));
+    button->SetHorizontalAlignment(gfx::ALIGN_CENTER);
+    button->SetEnabledColor(
+        ui::ColorVariant(kColorTabForegroundActiveFrameActive));
+    button->SetBackground(views::CreateRoundedRectBackground(
+        ui::ColorVariant(kColorWindriseAiPanelButtonBackground), radius));
+    return actions->AddChildView(std::move(button));
+  };
+  make_button(IDS_WINDRISE_AI_OPEN_APP, kActionOpenApp);
+  make_button(IDS_WINDRISE_AI_GALLERY, kActionGallery);
+  // 「设置」靠右（帧 36:26925 / 15:73440 都是这样排的）。
+  views::LabelButton* settings =
+      make_button(IDS_WINDRISE_AI_SETTINGS, kActionSettings);
+  views::View* spacer = actions->AddChildView(std::make_unique<views::View>());
+  actions->SetFlexForView(spacer, 1, true);
+  if (settings) {
+    actions->ReorderChildView(settings, actions->children().size() - 1);
+  }
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
+void WindriseAiPanelView::ShowAtAnchor(views::View* anchor) {
+  if (!anchor) {
+    return;
+  }
+  SetAnchorView(anchor);
+  SetArrow(views::BubbleBorder::BOTTOM_RIGHT);
+  // 折叠时向右浮出、宽 280；展开时与卡片同宽（224）。
+  set_fixed_width(anchor->width() >= 100 ? Metric(LayoutMetric::kAiPanelWidth)
+                                         : Metric(LayoutMetric::kAiPanelCollapsedWidth));
+  views::Widget* widget =
+      views::BubbleDialogDelegateView::CreateBubble(this);
+  widget->Show();
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
+          l10n_util::GetStringFUTF16(
+              IDS_WINDRISE_AI_MINUTES_AGO,
+              base::FormatNumber(connection.last_active_minutes)),
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
+  // 面板已经开出来时，扩展再推新数据要让气泡跟着长高（158 的 Widget 没有
+  // SizeToPreferredSize，用根视图的期望尺寸重设一次）。
+  if (views::Widget* widget = GetWidget()) {
+    widget->SetSize(widget->GetRootView()->GetPreferredSize());
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
