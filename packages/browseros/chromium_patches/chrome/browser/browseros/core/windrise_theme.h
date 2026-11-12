diff --git a/chrome/browser/browseros/core/windrise_theme.h b/chrome/browser/browseros/core/windrise_theme.h
new file mode 100644
index 0000000..1fbeade
--- /dev/null
+++ b/chrome/browser/browseros/core/windrise_theme.h
@@ -0,0 +1,181 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_THEME_H_
+#define CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_THEME_H_
+
+#include <cstddef>
+#include <memory>
+
+#include "base/functional/callback.h"
+#include "base/memory/raw_ptr.h"
+#include "components/prefs/pref_change_registrar.h"
+#include "third_party/skia/include/core/SkColor.h"
+
+class PrefService;
+
+namespace user_prefs {
+class PrefRegistrySyncable;
+}  // namespace user_prefs
+
+// 风桥外观数值的运行时来源。整窗配色（ColorMixer）、弹层圆角、工具栏格子
+// 按钮尺寸、网页区圆角都从这里取，值全部来自 profile 偏好：改偏好即改外观，
+// 不用重编。默认值是 docs 里的定稿数值（见报告「偏好键对照表」）。
+//
+// 颜色偏好存十进制 RGB（0xRRGGBB，例如 #E8EBEE 写 15264750），读出来统一补
+// 不透明 alpha。这样值恒为正数，user_prefs JSON 里可读。需要半透明时再加单独
+// 的 alpha 偏好。
+namespace browseros::windrise {
+
+// 整窗颜色 token。每个 token 一组亮 / 暗偏好。
+enum class ThemeToken {
+  kFrame = 0,           // 窗框、竖向标签栏底
+  kBar,                 // 工具栏、书签栏
+  kOmnibox,             // 地址栏底
+  kTabActive,           // 当前标签、浮出面板
+  kTile,                // 「+」、顶部小按钮、格子按钮、插槽卡片、选中行
+  kPopup,               // 菜单、气泡、对话框
+  kText,                // 正文、标签标题
+  kTextSecondary,       // 次要文字、未选中标签
+  kIcon,                // 工具栏图标
+  kLine,                // 分隔线
+  kWebContentsGap,      // 网页区四周的缝（N10 定：缝宽 0，颜色留着备用）
+  kPanel,               // 「已连接 N 个 AI」面板底（N7，B 版银色玻璃的平涂近似）
+  kCard,                // 左下卡片底（N7）
+  kSectionHeaderText,   // 侧边栏上下分段的段标题（N6）
+  kSectionDivider,      // 分段之间的分隔线（N6）
+  kSlotChipBackground,  // 闪办芯片底（墨色实心）
+  kSlotChipForeground,  // 闪办芯片字与图标
+  // N7 左下卡片与浮出面板（B 版银色玻璃，原生只能平涂）
+  kAiCardBackground,     // 卡片底
+  kAiPanelBackground,    // 面板底
+  kAiPanelButtonBackground,  // 面板按钮底（白玻璃的平涂近似）
+  // N11 银色海浪（取值来自 native/silver_wave.sksl 第四版里的常量）
+  kSilverWaveWash,        // 雾：浅银灰
+  kSilverWaveSoftDeep,    // 晕的暗处（深银）
+  kSilverWaveSoftBright,  // 晕的亮处
+  kSilverWaveCoreDeep,    // 芯的暗处
+  kSilverWaveCoreBright,  // 芯的亮处
+};
+inline constexpr size_t kThemeTokenCount =
+    static_cast<size_t>(ThemeToken::kSilverWaveCoreBright) + 1;
+
+// 布局数值 token（像素）。
+enum class LayoutMetric {
+  kRadiusSmall = 0,          // 输入框、行、提示
+  kRadiusMedium,             // 卡片、菜单、气泡
+  kRadiusLarge,              // 对话框、地址栏下拉
+  kGridButtonHeight,         // 工具栏格子按钮高
+  kGridButtonRadius,         // 工具栏格子按钮圆角（全圆角 = 高度 / 2）
+  kGridButtonPaddingLeft,    // 格子按钮左侧内边距
+  kGridButtonPaddingRight,   // 格子按钮右侧内边距
+  kGridButtonIconGap,        // 格子按钮图标与文字的间距
+  kGridButtonFontSize,       // 格子按钮文字字号
+  kWebContentsRadius,        // 网页区圆角
+  kWebContentsGap,           // 网页区与窗框之间的缝（N10 定：默认 0）
+  // N3 竖向标签栏
+  kTabHeight,                  // 标签高
+  kTabRadius,                  // 标签圆角
+  kTabStripHorizontalPadding,  // 标签列左右留白（标签宽 = 栏宽 - 两倍）
+  kTabContentsInsetLeft,       // 标签内左内边
+  kTabContentsInsetRight,      // 标签内右内边（关闭按钮外侧）
+  kTopButtonSize,              // 顶部见方小按钮（展开收起 / 格子 / 搜索）
+  kTopButtonIconSize,          // 顶部小按钮图标
+  kTopButtonRadius,            // 顶部小按钮圆角
+  kNewTabButtonHeight,         // 整宽「+」的高度
+  // N6 上下分段
+  kAiSectionHeaderHeight,   // 段标题行高
+  kAiSectionHeaderFontSize, // 段标题字号
+  // N7 左下「已连接 N 个 AI」卡片与面板
+  kAiCardHeight,           // 卡片高
+  kAiCardRadius,           // 卡片圆角
+  kAiCardPaddingLeft,      // 卡片左内边（AI 标志与面板文字同一条竖线）
+  kAiCardIconSize,         // 卡片里 AI 标志方块边长
+  kAiCardPanelGap,         // 卡片与浮出面板的间距
+  kAiPanelWidth,           // 面板宽（标签栏展开时）
+  kAiPanelCollapsedWidth,  // 面板宽（标签栏折叠、向右浮出时）
+  kAiPanelRadius,          // 面板圆角
+  kAiPanelPadding,         // 面板内边
+  kAiPanelRowHeight,       // 面板行高
+  kAiPanelButtonHeight,    // 面板按钮高
+  // N10 / N11
+  kWindowCornerRadius,       // 窗口圆角（Windows 11 系统裁出来的那 8px）
+  kSilverWaveSpread,         // 银浪层向网页区外扩的距离（外溢辉光）
+  kSilverWaveInwardMax,      // 银浪往里羽化的最大宽度
+  kSilverWaveOutwardMax,     // 银浪往外的最大宽度
+  kSilverWaveFadeInMs,       // 起：亮起时长
+  kSilverWaveFadeOutMs,      // 收：淡出时长
+  kSilverWaveIntroMs,        // 开场那一道沿边跑一圈的时长
+};
+inline constexpr size_t kLayoutMetricCount =
+    static_cast<size_t>(LayoutMetric::kSilverWaveIntroMs) + 1;
+
+// 颜色 token 的偏好名与编译期默认值。
+struct ThemeColorSpec {
+  ThemeToken token;
+  const char* light_pref;
+  const char* dark_pref;
+  int light_rgb;
+  int dark_rgb;
+};
+
+// 布局数值的偏好名与编译期默认值。
+struct LayoutMetricSpec {
+  LayoutMetric metric;
+  const char* pref;
+  int value;
+};
+
+// 两个表各自用 token 字段定位（不依赖数组顺序），供注册、读取与文档使用。
+extern const ThemeColorSpec kThemeColorSpecs[kThemeTokenCount];
+extern const LayoutMetricSpec kLayoutMetricSpecs[kLayoutMetricCount];
+
+// 一份颜色快照，按 ThemeToken 索引。
+struct ThemeColors {
+  SkColor color[kThemeTokenCount] = {};
+
+  SkColor Get(ThemeToken token) const {
+    return color[static_cast<size_t>(token)];
+  }
+};
+
+// 一份布局快照，按 LayoutMetric 索引。
+struct LayoutMetrics {
+  int value[kLayoutMetricCount] = {};
+
+  int Get(LayoutMetric metric) const {
+    return value[static_cast<size_t>(metric)];
+  }
+};
+
+// 注册风桥外观偏好（颜色 + 布局）。由 browseros::RegisterProfilePrefs 调用。
+void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
+
+// 把 `prefs` 里的外观数值装进进程级快照（UI 线程）。返回是否有值发生变化。
+bool LoadFromPrefs(const PrefService& prefs);
+
+// 读取当前快照（UI 线程）。没有偏好时是编译期默认值。
+const ThemeColors& GetLightColors();
+const ThemeColors& GetDarkColors();
+const LayoutMetrics& GetLayoutMetrics();
+
+// 观察外观偏好：构造时先装一次快照并注册全部键，之后任一键变化就重装快照
+// 并在真的变了时回调。生命周期跟持有者（ThemeService）。
+class ThemePrefObserver {
+ public:
+  ThemePrefObserver(PrefService* prefs, base::RepeatingClosure on_changed);
+  ThemePrefObserver(const ThemePrefObserver&) = delete;
+  ThemePrefObserver& operator=(const ThemePrefObserver&) = delete;
+  ~ThemePrefObserver();
+
+ private:
+  void OnPrefChanged();
+
+  raw_ptr<PrefService> prefs_;
+  base::RepeatingClosure on_changed_;
+  PrefChangeRegistrar registrar_;
+};
+
+}  // namespace browseros::windrise
+
+#endif  // CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_THEME_H_
