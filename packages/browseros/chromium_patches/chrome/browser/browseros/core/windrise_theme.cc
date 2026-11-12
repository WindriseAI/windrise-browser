diff --git a/chrome/browser/browseros/core/windrise_theme.cc b/chrome/browser/browseros/core/windrise_theme.cc
new file mode 100644
index 0000000000..1b636fa692
--- /dev/null
+++ b/chrome/browser/browseros/core/windrise_theme.cc
@@ -0,0 +1,202 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/browseros/core/windrise_theme.h"
+
+#include <utility>
+
+#include "base/functional/bind.h"
+#include "base/no_destructor.h"
+#include "components/pref_registry/pref_registry_syncable.h"
+#include "components/prefs/pref_service.h"
+
+namespace browseros::windrise {
+
+// 颜色默认值是 browser-theme-v2.md 第 2 节的定稿值（亮 / 暗）。偏好里存 RGB。
+const ThemeColorSpec kThemeColorSpecs[kThemeTokenCount] = {
+    {ThemeToken::kFrame, "browseros.windrise.theme.light.frame",
+     "browseros.windrise.theme.dark.frame", 0xDDE1E5, 0x262B32},
+    {ThemeToken::kBar, "browseros.windrise.theme.light.bar",
+     "browseros.windrise.theme.dark.bar", 0xEEF0F2, 0x2E343C},
+    {ThemeToken::kOmnibox, "browseros.windrise.theme.light.omnibox",
+     "browseros.windrise.theme.dark.omnibox", 0xFFFFFF, 0x20252B},
+    {ThemeToken::kTabActive, "browseros.windrise.theme.light.tab_active",
+     "browseros.windrise.theme.dark.tab_active", 0xF7F8F9, 0x3C434C},
+    {ThemeToken::kTile, "browseros.windrise.theme.light.tile",
+     "browseros.windrise.theme.dark.tile", 0xE8EBEE, 0x323840},
+    {ThemeToken::kPopup, "browseros.windrise.theme.light.popup",
+     "browseros.windrise.theme.dark.popup", 0xF7F8F9, 0x3C434C},
+    {ThemeToken::kText, "browseros.windrise.theme.light.text",
+     "browseros.windrise.theme.dark.text", 0x0E1116, 0xECEEF1},
+    {ThemeToken::kTextSecondary, "browseros.windrise.theme.light.text_secondary",
+     "browseros.windrise.theme.dark.text_secondary", 0x4A515C, 0xB4BCC5},
+    {ThemeToken::kIcon, "browseros.windrise.theme.light.icon",
+     "browseros.windrise.theme.dark.icon", 0x3C434D, 0xC4CAD2},
+    {ThemeToken::kLine, "browseros.windrise.theme.light.line",
+     "browseros.windrise.theme.dark.line", 0xC9CED5, 0x3A4048},
+    {ThemeToken::kWebContentsGap, "browseros.windrise.theme.light.web_gap",
+     "browseros.windrise.theme.dark.web_gap", 0xDDE1E5, 0x262B32},
+    // WINDRISE-TODO(value)：N7 定稿用 B 版银色玻璃（mock panelalt 是
+    // linear-gradient(180deg,#EEF0F2,#E3E7EB)）。原生 color id 只能平涂，
+    // 先取最接近的 --wr-tile 值，等设计帮手给平涂定值后改这里与偏好默认值。
+    {ThemeToken::kPanel, "browseros.windrise.theme.light.panel",
+     "browseros.windrise.theme.dark.panel", 0xE8EBEE, 0x323840},
+    {ThemeToken::kCard, "browseros.windrise.theme.light.card",
+     "browseros.windrise.theme.dark.card", 0xE8EBEE, 0x323840},
+    // WINDRISE-TODO(value)：N6 段标题 / 分隔线的定值还没给，先用次要文字与线。
+    {ThemeToken::kSectionHeaderText,
+     "browseros.windrise.theme.light.section_header_text",
+     "browseros.windrise.theme.dark.section_header_text", 0x4A515C, 0xB4BCC5},
+    {ThemeToken::kSectionDivider,
+     "browseros.windrise.theme.light.section_divider",
+     "browseros.windrise.theme.dark.section_divider", 0xC9CED5, 0x3A4048},
+    {ThemeToken::kSlotChipBackground,
+     "browseros.windrise.theme.light.slot_chip_background",
+     "browseros.windrise.theme.dark.slot_chip_background", 0x14171C, 0x14171C},
+    {ThemeToken::kSlotChipForeground,
+     "browseros.windrise.theme.light.slot_chip_foreground",
+     "browseros.windrise.theme.dark.slot_chip_foreground", 0xFFFFFF, 0xFFFFFF},
+};
+
+// 布局默认值来自 browser-theme-v2.md 第 3 节与内部页样式表（圆角 s/m/l）。
+const LayoutMetricSpec kLayoutMetricSpecs[kLayoutMetricCount] = {
+    {LayoutMetric::kRadiusSmall, "browseros.windrise.layout.radius_small", 8},
+    {LayoutMetric::kRadiusMedium, "browseros.windrise.layout.radius_medium", 12},
+    {LayoutMetric::kRadiusLarge, "browseros.windrise.layout.radius_large", 16},
+    {LayoutMetric::kGridButtonHeight,
+     "browseros.windrise.layout.grid_button_height", 32},
+    {LayoutMetric::kGridButtonRadius,
+     "browseros.windrise.layout.grid_button_radius", 16},
+    {LayoutMetric::kGridButtonPaddingLeft,
+     "browseros.windrise.layout.grid_button_padding_left", 10},
+    {LayoutMetric::kGridButtonPaddingRight,
+     "browseros.windrise.layout.grid_button_padding_right", 12},
+    {LayoutMetric::kGridButtonIconGap,
+     "browseros.windrise.layout.grid_button_icon_gap", 6},
+    {LayoutMetric::kGridButtonFontSize,
+     "browseros.windrise.layout.grid_button_font_size", 14},
+    {LayoutMetric::kWebContentsRadius,
+     "browseros.windrise.layout.web_contents_radius", 12},
+    // N10 定稿：网页区四边的缝为 0（还剩圆角）。留着这个键是为了以后要缝时
+    // 改偏好即可，不用重编。
+    {LayoutMetric::kWebContentsGap, "browseros.windrise.layout.web_contents_gap",
+     0},
+};
+
+namespace {
+
+struct Snapshot {
+  ThemeColors light;
+  ThemeColors dark;
+  LayoutMetrics layout;
+};
+
+// 偏好里只存 RGB，读出来补不透明 alpha。
+SkColor FromRgbPref(int rgb) {
+  return SkColorSetRGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
+}
+
+Snapshot MakeDefaultSnapshot() {
+  Snapshot snapshot;
+  // 按 spec 里的 token 落位，不依赖数组顺序。
+  for (const ThemeColorSpec& spec : kThemeColorSpecs) {
+    const size_t index = static_cast<size_t>(spec.token);
+    snapshot.light.color[index] = FromRgbPref(spec.light_rgb);
+    snapshot.dark.color[index] = FromRgbPref(spec.dark_rgb);
+  }
+  for (const LayoutMetricSpec& spec : kLayoutMetricSpecs) {
+    snapshot.layout.value[static_cast<size_t>(spec.metric)] = spec.value;
+  }
+  return snapshot;
+}
+
+Snapshot& GetSnapshot() {
+  static base::NoDestructor<Snapshot> snapshot(MakeDefaultSnapshot());
+  return *snapshot;
+}
+
+}  // namespace
+
+void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
+  for (const ThemeColorSpec& spec : kThemeColorSpecs) {
+    registry->RegisterIntegerPref(spec.light_pref, spec.light_rgb);
+    registry->RegisterIntegerPref(spec.dark_pref, spec.dark_rgb);
+  }
+  for (const LayoutMetricSpec& spec : kLayoutMetricSpecs) {
+    registry->RegisterIntegerPref(spec.pref, spec.value);
+  }
+}
+
+bool LoadFromPrefs(const PrefService& prefs) {
+  Snapshot& snapshot = GetSnapshot();
+  bool changed = false;
+
+  for (const ThemeColorSpec& spec : kThemeColorSpecs) {
+    const size_t index = static_cast<size_t>(spec.token);
+    const SkColor light = FromRgbPref(prefs.GetInteger(spec.light_pref));
+    if (snapshot.light.color[index] != light) {
+      snapshot.light.color[index] = light;
+      changed = true;
+    }
+    const SkColor dark = FromRgbPref(prefs.GetInteger(spec.dark_pref));
+    if (snapshot.dark.color[index] != dark) {
+      snapshot.dark.color[index] = dark;
+      changed = true;
+    }
+  }
+
+  for (const LayoutMetricSpec& spec : kLayoutMetricSpecs) {
+    const size_t index = static_cast<size_t>(spec.metric);
+    const int value = prefs.GetInteger(spec.pref);
+    if (snapshot.layout.value[index] != value) {
+      snapshot.layout.value[index] = value;
+      changed = true;
+    }
+  }
+
+  return changed;
+}
+
+const ThemeColors& GetLightColors() {
+  return GetSnapshot().light;
+}
+
+const ThemeColors& GetDarkColors() {
+  return GetSnapshot().dark;
+}
+
+const LayoutMetrics& GetLayoutMetrics() {
+  return GetSnapshot().layout;
+}
+
+ThemePrefObserver::ThemePrefObserver(PrefService* prefs,
+                                     base::RepeatingClosure on_changed)
+    : prefs_(prefs), on_changed_(std::move(on_changed)) {
+  // 先装一次，保证第一帧就用偏好里的值。
+  LoadFromPrefs(*prefs_);
+
+  registrar_.Init(prefs_);
+  for (const ThemeColorSpec& spec : kThemeColorSpecs) {
+    registrar_.Add(spec.light_pref,
+                   base::BindRepeating(&ThemePrefObserver::OnPrefChanged,
+                                       base::Unretained(this)));
+    registrar_.Add(spec.dark_pref,
+                   base::BindRepeating(&ThemePrefObserver::OnPrefChanged,
+                                       base::Unretained(this)));
+  }
+  for (const LayoutMetricSpec& spec : kLayoutMetricSpecs) {
+    registrar_.Add(spec.pref,
+                   base::BindRepeating(&ThemePrefObserver::OnPrefChanged,
+                                       base::Unretained(this)));
+  }
+}
+
+ThemePrefObserver::~ThemePrefObserver() = default;
+
+void ThemePrefObserver::OnPrefChanged() {
+  if (LoadFromPrefs(*prefs_) && on_changed_) {
+    on_changed_.Run();
+  }
+}
+
+}  // namespace browseros::windrise
