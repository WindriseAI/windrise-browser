diff --git a/chrome/browser/ui/views/toolbar/windrise_grid_button.cc b/chrome/browser/ui/views/toolbar/windrise_grid_button.cc
new file mode 100644
index 0000000000..f4f9157fe9
--- /dev/null
+++ b/chrome/browser/ui/views/toolbar/windrise_grid_button.cc
@@ -0,0 +1,178 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/toolbar/windrise_grid_button.h"
+
+#include <algorithm>
+#include <memory>
+#include <utility>
+
+#include "base/functional/bind.h"
+#include "base/memory/ref_counted.h"
+#include "chrome/app/vector_icons/vector_icons.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/extensions/windrise_extension_protection.h"
+#include "chrome/browser/profiles/profile.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
+#include "chrome/browser/ui/singleton_tabs.h"
+#include "chrome/common/webui_url_constants.h"
+#include "chrome/grit/generated_resources.h"
+#include "extensions/browser/event_router.h"
+#include "extensions/browser/extension_event_histogram_value.h"
+#include "extensions/browser/extension_registry.h"
+#include "ui/base/l10n/l10n_util.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/gfx/font_list.h"
+#include "ui/gfx/geometry/insets.h"
+#include "ui/views/accessibility/view_accessibility.h"
+#include "ui/views/controls/label.h"
+#include "url/gurl.h"
+
+namespace {
+
+using browseros::windrise::LayoutMetric;
+
+// 工具栏「格子」按钮点下去后扩展收到的事件名。扩展侧用
+// chrome.windrise.onGridButtonClicked 监听（见 windrise.idl）。
+constexpr char kGridClickedEventName[] = "windrise.onGridButtonClicked";
+constexpr char kWindriseGridsFallbackUrl[] = "chrome://windrise/settings#grids";
+
+}  // namespace
+
+WindriseGridButton::WindriseGridButton(BrowserWindowInterface* browser)
+    : ToolbarButton(base::BindRepeating(
+          &WindriseGridButton::NotifyExtensionGridClicked,
+          base::Unretained(this))),
+      browser_(browser) {
+  SetText(l10n_util::GetStringUTF16(IDS_WINDRISE_GRID));
+  SetTooltipText(std::u16string(GetText()));
+  // 窄窗时字拿掉，读屏名字仍是「格子」。
+  GetViewAccessibility().SetName(std::u16string(GetText()));
+  SetDefaultBackgroundColorId(kColorWindriseTileBackground);
+  SetVectorIcon(kGridViewIcon);
+  ApplyStyleFromPrefs();
+  // 格子模式是普通浏览窗口的功能；弹窗 / 应用窗口的工具栏不放这个按钮。
+  SetVisible(browser_ && browser_->GetType() == BrowserWindowInterface::TYPE_NORMAL);
+}
+
+WindriseGridButton::~WindriseGridButton() = default;
+
+void WindriseGridButton::OnThemeChanged() {
+  ToolbarButton::OnThemeChanged();
+  UpdateIconsWithStandardColors(kGridViewIcon);
+  // 颜色缓存被清掉时也会走到这里，顺手重读一遍尺寸类偏好。
+  ApplyStyleFromPrefs();
+}
+
+int WindriseGridButton::GetRoundedCornerRadius() const {
+  return browseros::windrise::GetLayoutMetrics().Get(
+      LayoutMetric::kGridButtonRadius);
+}
+
+void WindriseGridButton::AddedToWidget() {
+  ToolbarButton::AddedToWidget();
+  views::Widget* widget = GetWidget();
+  if (widget && !widget_observation_.IsObservingSource(widget)) {
+    widget_observation_.Observe(widget);
+  }
+  if (widget && !widget->IsMinimized()) {
+    UpdateCompact(widget->GetWindowBoundsInScreen().width());
+  }
+}
+
+void WindriseGridButton::RemovedFromWidget() {
+  widget_observation_.Reset();
+  ToolbarButton::RemovedFromWidget();
+}
+
+void WindriseGridButton::OnWidgetBoundsChanged(views::Widget* widget,
+                                               const gfx::Rect& new_bounds) {
+  // 最小化时报上来的宽不算（同侧栏）。
+  if (widget->IsMinimized()) {
+    return;
+  }
+  UpdateCompact(new_bounds.width());
+}
+
+void WindriseGridButton::OnWidgetDestroying(views::Widget* widget) {
+  widget_observation_.Reset();
+}
+
+void WindriseGridButton::UpdateCompact(int window_width) {
+  if (window_width <= 0) {
+    return;
+  }
+  const bool compact =
+      browseros::windrise::IsNarrowWindow(window_width, compact_);
+  if (compact == compact_) {
+    return;
+  }
+  compact_ = compact;
+  ApplyStyleFromPrefs();
+}
+
+void WindriseGridButton::ApplyStyleFromPrefs() {
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const int height = metrics.Get(LayoutMetric::kGridButtonHeight);
+
+  if (compact_) {
+    // 只剩图标：左右内边距 = (高 - 图标) / 2，整颗是高 x 高的圆（圆角偏好
+    // grid_button_radius 默认就是半高，全圆角）。
+    SetText(std::u16string());
+    const int side = std::max(0, (height - GetIconSize()) / 2);
+    SetLayoutInsets(gfx::Insets::TLBR(0, side, 0, side));
+  } else {
+    SetText(l10n_util::GetStringUTF16(IDS_WINDRISE_GRID));
+    SetLayoutInsets(gfx::Insets::TLBR(
+        0, metrics.Get(LayoutMetric::kGridButtonPaddingLeft), 0,
+        metrics.Get(LayoutMetric::kGridButtonPaddingRight)));
+  }
+  SetMinSize(gfx::Size(0, height));
+  // LabelButton::SetText 会把读屏名字一起改成按钮上的字（窄窗时是空的），
+  // 每次换字以后把名字放回「格子」。
+  GetViewAccessibility().SetName(
+      l10n_util::GetStringUTF16(IDS_WINDRISE_GRID));
+  SetImageLabelSpacing(metrics.Get(LayoutMetric::kGridButtonIconGap));
+
+  const int font_size = metrics.Get(LayoutMetric::kGridButtonFontSize);
+  const gfx::FontList& font_list = label()->font_list();
+  if (font_list.GetFontSize() != font_size) {
+    label()->SetFontList(
+        font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize()));
+  }
+
+  UpdateColorsAndInsets();
+  PreferredSizeChanged();
+}
+
+void WindriseGridButton::NotifyExtensionGridClicked() {
+  Profile* profile = browser_ ? browser_->GetProfile() : nullptr;
+  if (!profile) {
+    return;
+  }
+  auto* registry = extensions::ExtensionRegistry::Get(profile);
+  const extensions::Extension* assistant =
+      registry ? registry->enabled_extensions().GetByID(
+                     browseros::kWindriseAssistantExtensionId)
+               : nullptr;
+  // 桌面端在线（持有有效租约）时照常广播事件给扩展，由扩展转发 grid_open 弹全屏 grids.html；
+  // 独立启动浏览器或桌面端未在线时，不再静默无反应，改为打开扩展内的格子管理页。
+  if (assistant && extensions::IsAuthorizedWindriseExtension(assistant)) {
+    auto* event_router = extensions::EventRouter::Get(profile);
+    if (event_router) {
+      event_router->BroadcastEvent(std::make_unique<extensions::Event>(
+          extensions::events::UNKNOWN, kGridClickedEventName,
+          base::MakeRefCounted<extensions::EventArgs>(), profile));
+      return;
+    }
+  }
+  ShowSingletonTab(browser_,
+                   GURL(assistant ? kWindriseGridsFallbackUrl
+                                  : chrome::kChromeUISettingsURL));
+}
+
+BEGIN_METADATA(WindriseGridButton)
+END_METADATA
