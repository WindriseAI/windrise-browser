diff --git a/chrome/browser/ui/color/windrise_color_mixer.cc b/chrome/browser/ui/color/windrise_color_mixer.cc
new file mode 100644
index 0000000000..62b6aaaad0
--- /dev/null
+++ b/chrome/browser/ui/color/windrise_color_mixer.cc
@@ -0,0 +1,173 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/color/windrise_color_mixer.h"
+
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/ui/color/chrome_color_id.h"
+#include "ui/color/color_id.h"
+#include "ui/color/color_mixer.h"
+#include "ui/color/color_provider.h"
+#include "ui/color/color_recipe.h"
+
+namespace {
+
+using browseros::windrise::ThemeColors;
+using browseros::windrise::ThemeToken;
+
+// 无痕窗口独立深石墨银配色：与普通浅色窗（#DDE1E5/#EEF0F2）及普通深色窗
+// （#262B32/#2E343C）均有清晰视觉区分，保持无彩色银灰体系与高可读对比度。
+const ThemeColors& GetIncognitoColors() {
+  static const ThemeColors kIncognito = [] {
+    ThemeColors colors;
+    const auto set = [&colors](ThemeToken token, SkColor value) {
+      colors.color[static_cast<size_t>(token)] = value;
+    };
+    set(ThemeToken::kFrame, SkColorSetRGB(0x16, 0x19, 0x1D));
+    set(ThemeToken::kBar, SkColorSetRGB(0x1E, 0x22, 0x28));
+    set(ThemeToken::kOmnibox, SkColorSetRGB(0x12, 0x15, 0x19));
+    set(ThemeToken::kTabActive, SkColorSetRGB(0x2A, 0x30, 0x38));
+    set(ThemeToken::kTile, SkColorSetRGB(0x24, 0x29, 0x30));
+    set(ThemeToken::kPopup, SkColorSetRGB(0x23, 0x28, 0x30));
+    set(ThemeToken::kText, SkColorSetRGB(0xF2, 0xF4, 0xF7));
+    set(ThemeToken::kTextSecondary, SkColorSetRGB(0x9A, 0xA3, 0xAE));
+    set(ThemeToken::kIcon, SkColorSetRGB(0xD0, 0xD6, 0xDE));
+    set(ThemeToken::kLine, SkColorSetRGB(0x34, 0x3B, 0x44));
+    set(ThemeToken::kWebContentsGap, SkColorSetRGB(0x16, 0x19, 0x1D));
+    set(ThemeToken::kPanel, SkColorSetRGB(0x23, 0x28, 0x30));
+    set(ThemeToken::kCard, SkColorSetRGB(0x23, 0x28, 0x30));
+    set(ThemeToken::kSectionHeaderText, SkColorSetRGB(0x9A, 0xA3, 0xAE));
+    set(ThemeToken::kSectionDivider, SkColorSetRGB(0x34, 0x3B, 0x44));
+    set(ThemeToken::kSlotChipBackground, SkColorSetRGB(0x0E, 0x10, 0x13));
+    set(ThemeToken::kSlotChipForeground, SkColorSetRGB(0xF2, 0xF4, 0xF7));
+    set(ThemeToken::kAiCardBackground, SkColorSetRGB(0x23, 0x28, 0x30));
+    set(ThemeToken::kAiPanelBackground, SkColorSetRGB(0x23, 0x28, 0x30));
+    set(ThemeToken::kAiPanelButtonBackground, SkColorSetRGB(0x34, 0x3B, 0x44));
+    set(ThemeToken::kAiCardBackgroundTop, SkColorSetRGB(0x2A, 0x30, 0x38));
+    set(ThemeToken::kAiCardBackgroundBottom, SkColorSetRGB(0x1E, 0x22, 0x28));
+    set(ThemeToken::kAiPanelBackgroundTop, SkColorSetRGB(0x2A, 0x30, 0x38));
+    set(ThemeToken::kAiPanelBackgroundBottom, SkColorSetRGB(0x1E, 0x22, 0x28));
+    return colors;
+  }();
+  return kIncognito;
+}
+
+}  // namespace
+
+void AddWindriseColorMixer(ui::ColorProvider* provider,
+                           const ui::ColorProviderKey& key) {
+  const bool dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;
+  // ThemeService::GetColorProviderKey 对无痕 Profile 固定设置
+  // kDark + UserColorSource::kGrayscale + custom_theme == nullptr；
+  // 风桥普通窗口由 SyncDefaultTheme 置 kUserColor（走 kAccent），据此可靠区分
+  // 无痕窗口与普通深浅色窗口，避免无痕与普通深色 100% 同色。
+  const bool incognito =
+      dark &&
+      key.user_color_source ==
+          ui::ColorProviderKey::UserColorSource::kGrayscale &&
+      !key.custom_theme;
+  const ThemeColors& colors =
+      incognito ? GetIncognitoColors()
+                : (dark ? browseros::windrise::GetDarkColors()
+                        : browseros::windrise::GetLightColors());
+  const auto token = [&colors](ThemeToken id) { return colors.Get(id); };
+
+  ui::ColorMixer& mixer = provider->AddMixer();
+
+  // 窗框与标签栏底．
+  mixer[ui::kColorFrameActive] = {token(ThemeToken::kFrame)};
+  mixer[ui::kColorFrameInactive] = {token(ThemeToken::kFrame)};
+
+  // 工具栏与书签栏．
+  mixer[kColorToolbar] = {token(ThemeToken::kBar)};
+  mixer[kColorBookmarkBarBackground] = {token(ThemeToken::kBar)};
+  mixer[kColorBookmarkBarForeground] = {token(ThemeToken::kText)};
+  mixer[kColorToolbarText] = {token(ThemeToken::kText)};
+  mixer[kColorToolbarButtonIcon] = {token(ThemeToken::kIcon)};
+  mixer[kColorToolbarButtonIconHovered] = {token(ThemeToken::kIcon)};
+  mixer[kColorToolbarButtonBackgroundHighlightedDefault] = {
+      token(ThemeToken::kTile)};
+
+  // 地址栏．悬停取值方案里没有，先跟常态一致。
+  mixer[kColorLocationBarBackground] = {token(ThemeToken::kOmnibox)};
+  mixer[kColorLocationBarBackgroundHovered] = {token(ThemeToken::kOmnibox)};
+  mixer[kColorOmniboxText] = {token(ThemeToken::kText)};
+  mixer[kColorOmniboxResultsBackground] = {token(ThemeToken::kPopup)};
+  mixer[kColorOmniboxResultsBackgroundSelected] = {token(ThemeToken::kTile)};
+
+  // 标签．
+  mixer[kColorTabBackgroundActiveFrameActive] = {
+      token(ThemeToken::kTabActive)};
+  mixer[kColorTabBackgroundActiveFrameInactive] = {
+      token(ThemeToken::kTabActive)};
+  mixer[kColorTabBackgroundInactiveFrameActive] = {token(ThemeToken::kFrame)};
+  mixer[kColorTabBackgroundInactiveFrameInactive] = {token(ThemeToken::kFrame)};
+  mixer[kColorTabForegroundActiveFrameActive] = {token(ThemeToken::kText)};
+  mixer[kColorTabForegroundActiveFrameInactive] = {token(ThemeToken::kText)};
+  mixer[kColorTabForegroundInactiveFrameActive] = {
+      token(ThemeToken::kTextSecondary)};
+  mixer[kColorTabForegroundInactiveFrameInactive] = {
+      token(ThemeToken::kTextSecondary)};
+
+  // 分隔线．
+  mixer[ui::kColorSeparator] = {token(ThemeToken::kLine)};
+  mixer[kColorToolbarContentAreaSeparator] = {token(ThemeToken::kLine)};
+  mixer[kColorToolbarSeparator] = {token(ThemeToken::kLine)};
+  mixer[kColorToolbarTopSeparatorFrameActive] = {token(ThemeToken::kLine)};
+
+  // 弹层：菜单、气泡、对话框．圆角走 ChromeLayoutProvider。
+  mixer[ui::kColorMenuBackground] = {token(ThemeToken::kPopup)};
+  mixer[ui::kColorMenuSelectionBackground] = {token(ThemeToken::kTile)};
+  mixer[ui::kColorMenuItemBackgroundSelected] = {token(ThemeToken::kTile)};
+  mixer[ui::kColorMenuSeparator] = {token(ThemeToken::kLine)};
+  mixer[ui::kColorBubbleBackground] = {token(ThemeToken::kPopup)};
+  mixer[ui::kColorDialogBackground] = {token(ThemeToken::kPopup)};
+
+  // 风桥自有 color id：卡片、面板、标签 tile、网页区缝、分段、闪办芯片。
+  // 卡片 / 面板按 B 版银色玻璃的平涂近似，默认值见 windrise_theme.cc 的
+  // WINDRISE-TODO(value)。
+  mixer[kColorWindriseCardBackground] = {token(ThemeToken::kCard)};
+  mixer[kColorWindrisePanelBackground] = {token(ThemeToken::kPanel)};
+  mixer[kColorWindriseTabActiveBackground] = {token(ThemeToken::kTabActive)};
+  mixer[kColorWindriseTabInactiveBackground] = {token(ThemeToken::kFrame)};
+  mixer[kColorWindriseTileBackground] = {token(ThemeToken::kTile)};
+  mixer[kColorWindriseDivider] = {token(ThemeToken::kLine)};
+  mixer[kColorWindriseWebContentsGap] = {token(ThemeToken::kWebContentsGap)};
+  mixer[kColorWindriseSectionHeaderText] = {
+      token(ThemeToken::kSectionHeaderText)};
+  mixer[kColorWindriseSectionDivider] = {token(ThemeToken::kSectionDivider)};
+  mixer[kColorWindriseSlotChipBackground] = {
+      token(ThemeToken::kSlotChipBackground)};
+  mixer[kColorWindriseSlotChipForeground] = {
+      token(ThemeToken::kSlotChipForeground)};
+
+  // 左下「已连接 N 个 AI」卡片与浮出面板（B 版银色玻璃的平涂近似）。
+  mixer[kColorWindriseAiCardBackground] = {token(ThemeToken::kAiCardBackground)};
+  mixer[kColorWindriseAiCardBackgroundTop] = {
+      token(ThemeToken::kAiCardBackgroundTop)};
+  mixer[kColorWindriseAiCardBackgroundBottom] = {
+      token(ThemeToken::kAiCardBackgroundBottom)};
+  mixer[kColorWindriseAiPanelBackground] = {
+      token(ThemeToken::kAiPanelBackground)};
+  mixer[kColorWindriseAiPanelButtonBackground] = {
+      token(ThemeToken::kAiPanelButtonBackground)};
+  mixer[kColorWindriseAiPanelBackgroundTop] = {
+      token(ThemeToken::kAiPanelBackgroundTop)};
+  mixer[kColorWindriseAiPanelBackgroundBottom] = {
+      token(ThemeToken::kAiPanelBackgroundBottom)};
+
+  // WebUI 的 --color-sys-* 在 Windows 上不由 theme_source 下发，页面里用
+  // 的是 cr_shared_vars.css 的兜底值；这里把同一组 ui 颜色也按偏好给出，凡是
+  // 走 ColorProvider 的 WebUI 部件（以及后续要下发 sys 集合的平台）都一致。
+  mixer[ui::kColorSysPrimary] = {token(ThemeToken::kText)};
+  mixer[ui::kColorSysOnPrimary] = {token(ThemeToken::kTabActive)};
+  mixer[ui::kColorSysTonalContainer] = {token(ThemeToken::kTile)};
+  mixer[ui::kColorSysOnTonalContainer] = {token(ThemeToken::kText)};
+  mixer[ui::kColorSysDivider] = {token(ThemeToken::kLine)};
+  mixer[ui::kColorSysStateFocusRing] = {token(ThemeToken::kText)};
+  mixer[ui::kColorSysOnSurface] = {token(ThemeToken::kText)};
+  mixer[ui::kColorSysOnSurfaceSubtle] = {token(ThemeToken::kTextSecondary)};
+  mixer[ui::kColorSysNeutralContainer] = {token(ThemeToken::kTile)};
+  mixer[ui::kColorSysBaseContainer] = {token(ThemeToken::kPopup)};
+  mixer[ui::kColorSysSurface] = {token(ThemeToken::kPopup)};
+}
