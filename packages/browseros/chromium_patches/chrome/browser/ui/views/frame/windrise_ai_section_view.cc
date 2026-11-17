diff --git a/chrome/browser/ui/views/frame/windrise_ai_section_view.cc b/chrome/browser/ui/views/frame/windrise_ai_section_view.cc
new file mode 100644
index 0000000000..dab150e4f9
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_ai_section_view.cc
@@ -0,0 +1,2057 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/frame/windrise_ai_section_view.h"
+
+#include <algorithm>
+#include <array>
+#include <cmath>
+#include <cstdint>
+#include <cstdlib>
+#include <set>
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
+#include "chrome/browser/ui/tabs/alert/tab_alert_controller.h"
+#include "chrome/browser/ui/tabs/alert/tab_alert_icon.h"
+#include "chrome/browser/ui/tabs/vertical_tab_strip_state.h"
+#include "chrome/grit/generated_resources.h"
+#include "components/prefs/pref_service.h"
+#include "components/tabs/public/tab_alert.h"
+#include "components/tabs/public/tab_group.h"
+#include "components/tabs/public/tab_network_state.h"
+#include "components/vector_icons/vector_icons.h"
+#include "third_party/skia/include/core/SkColor.h"
+#include "third_party/skia/include/core/SkPoint.h"
+#include "third_party/skia/include/core/SkRRect.h"
+#include "third_party/skia/include/core/SkTileMode.h"
+#include "url/gurl.h"
+#include "ui/base/cursor/cursor.h"
+#include "ui/base/l10n/l10n_util.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/base/models/image_model.h"
+#include "ui/color/color_provider.h"
+#include "ui/compositor/compositor.h"
+#include "ui/compositor/layer.h"
+#include "ui/events/event.h"
+#include "ui/gfx/animation/animation.h"
+#include "ui/gfx/animation/linear_animation.h"
+#include "ui/gfx/animation/tween.h"
+#include "ui/gfx/canvas.h"
+#include "ui/gfx/color_utils.h"
+#include "ui/gfx/font.h"
+#include "ui/gfx/font_list.h"
+#include "ui/gfx/geometry/point_conversions.h"
+#include "ui/gfx/geometry/rect.h"
+#include "ui/gfx/geometry/rect_f.h"
+#include "ui/gfx/geometry/skia_conversions.h"
+#include "ui/gfx/image/image.h"
+#include "ui/gfx/paint_throbber.h"
+#include "ui/gfx/paint_vector_icon.h"
+#include "ui/gfx/scoped_canvas.h"
+#include "ui/gfx/text_utils.h"
+#include "ui/base/cursor/mojom/cursor_type.mojom-shared.h"
+#include "ui/views/accessibility/view_accessibility.h"
+#include "ui/views/context_menu_controller.h"
+#include "ui/views/controls/button/button.h"
+#include "ui/views/view_class_properties.h"
+#include "ui/views/widget/widget.h"
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
+// 滚轮方向和上段一致（+1）。
+constexpr int kWheelDirection = 1;
+
+// 动效曲线都是 WINDRISE_EASE_OUT
+// （.23, 1, .32, 1）；减少动态效果时 RichAnimationDuration 给 0，直接到位。
+constexpr base::TimeDelta kHeightDuration = base::Milliseconds(200);
+constexpr base::TimeDelta kSplitterFadeDuration = base::Milliseconds(150);
+constexpr base::TimeDelta kGripDuration = base::Milliseconds(150);
+constexpr base::TimeDelta kIndicatorInDuration = base::Milliseconds(180);
+constexpr base::TimeDelta kIndicatorOutDuration = base::Milliseconds(400);
+constexpr base::TimeDelta kIndicatorHold = base::Milliseconds(900);
+// 动画节拍：在窗口里时跟合成器的帧走（120Hz 屏 120 帧、60Hz 屏 60 帧，按时间
+// 推进）；还没进窗口时退回 LinearAnimation 自己的定时器（最快 100 帧）。
+constexpr int kTransitionFrameRate = 120;
+
+// 「正在操作」光效。起 300ms、收 400ms、曲线 WINDRISE_EASE_OUT：和网页
+// 四周光效的起 / 收同一组数（windrise_edge_light_view.h「开 300ms、收 400ms」）。
+// 这是一圈常驻的状态光，不是点一下的反馈，所以不受「UI 动画不超过 300ms」那条
+// 约束；减少动态效果时直接到位。
+constexpr base::TimeDelta kGlowInDuration = base::Milliseconds(300);
+constexpr base::TimeDelta kGlowOutDuration = base::Milliseconds(400);
+// 减少动态效果：光不流动，定格在这一刻（同网页光效 kEdgeLightFreezeTime）。
+constexpr float kGlowFreezeTime = 2.35f;
+// 沿一圈取多少个亮度点（扫角渐变的色标数）。
+constexpr int kGlowStops = 48;
+// 托底冷灰（同网页光效浅色页的托底柔影 rgba(132,143,158)，liuguang.js render 第 1 层）。
+constexpr SkColor kGlowBedColor = SkColorSetRGB(132, 143, 158);
+// 和上段标签的提示图标之间的间距（上游 tab_view_vertical_layout.cc kDefaultPadding）。
+constexpr int kIndicatorGap = 4;
+
+// 官网白边流光的起伏函数，和网页四周光效逐字相同（extension edge-light/
+// liuguang.js 的 wave()、windrise_edge_light_model.cc Wave()；后者已不编译，所以
+// 这里照抄一份，系数一个不改）。s 是沿一圈的位置 0..1，dir 是方向。两个方向
+// 叠起来用：四周同时都有亮处、亮处原地起伏，不是一段光绕圈。
+float GlowWave(float s, float t, float dir) {
+  constexpr float kTwoPi = 6.28318530718f;
+  const float a =
+      0.5f + 0.5f * std::sin(kTwoPi * (2.0f * s + dir * 0.09f * t) +
+                             1.4f * std::sin(kTwoPi * (3.0f * s - dir * 0.05f * t)));
+  const float b =
+      0.5f + 0.5f * std::sin(kTwoPi * (5.0f * s - dir * 0.13f * t) +
+                             0.8f * std::sin(kTwoPi * 2.0f * s + 0.3f * t));
+  return a * a * 0.75f + b * b * b * 0.45f;
+}
+
+// smoothstep(0.2, 0.9, x)。
+float SmoothStepForGlow(float x) {
+  const float e = std::clamp((x - 0.2f) / 0.7f, 0.0f, 1.0f);
+  return e * e * (3.0f - 2.0f * e);
+}
+
+// 圆角矩形一圈上按弧长均匀取的第 s 个点（s ∈ [0, 1]），从右边中点出发顺时针
+// （屏幕坐标 y 朝下，和扫角渐变的角度方向一致）。光效的亮度按弧长 s 算，
+// 再换成这个点相对中心的角度当扫角渐变色标的位置——否则按角度均分时，长边
+// 中段会被拉开、靠角的地方挤在一起，起伏看起来像绕中心转的辐条。
+gfx::PointF PerimeterPoint(const gfx::RectF& r, float radius, float s) {
+  constexpr float kHalfPi = 1.57079632679f;
+  const float rad = std::clamp(radius, 0.0f,
+                               std::min(r.width(), r.height()) / 2.0f);
+  const float lv = r.height() - 2.0f * rad;
+  const float lh = r.width() - 2.0f * rad;
+  const float arc = kHalfPi * rad;
+  float d = std::clamp(s, 0.0f, 1.0f) * (2.0f * lv + 2.0f * lh + 4.0f * arc);
+  const float cy = r.CenterPoint().y();
+  // 右边下半段
+  if (d <= lv / 2.0f) {
+    return gfx::PointF(r.right(), cy + d);
+  }
+  d -= lv / 2.0f;
+  const auto corner = [&](float ccx, float ccy, float start_angle,
+                          float dist) {
+    const float a = start_angle + (rad > 0.0f ? dist / rad : 0.0f);
+    return gfx::PointF(ccx + rad * std::cos(a), ccy + rad * std::sin(a));
+  };
+  if (d <= arc) {  // 右下角：从 0 转到 90 度
+    return corner(r.right() - rad, r.bottom() - rad, 0.0f, d);
+  }
+  d -= arc;
+  if (d <= lh) {  // 下边，从右往左
+    return gfx::PointF(r.right() - rad - d, r.bottom());
+  }
+  d -= lh;
+  if (d <= arc) {  // 左下角：90 到 180 度
+    return corner(r.x() + rad, r.bottom() - rad, kHalfPi, d);
+  }
+  d -= arc;
+  if (d <= lv) {  // 左边，从下往上
+    return gfx::PointF(r.x(), r.bottom() - rad - d);
+  }
+  d -= lv;
+  if (d <= arc) {  // 左上角：180 到 270 度
+    return corner(r.x() + rad, r.y() + rad, 2.0f * kHalfPi, d);
+  }
+  d -= arc;
+  if (d <= lh) {  // 上边，从左往右
+    return gfx::PointF(r.x() + rad + d, r.y());
+  }
+  d -= lh;
+  if (d <= arc) {  // 右上角：270 到 360 度
+    return corner(r.right() - rad, r.y() + rad, 3.0f * kHalfPi, d);
+  }
+  d -= arc;
+  // 右边上半段
+  return gfx::PointF(r.right(), r.y() + rad + d);
+}
+
+// 圆角矩形一圈的周长（和 PerimeterPoint 同一走法）。
+float PerimeterLength(const gfx::RectF& r, float radius) {
+  const float rad = std::clamp(radius, 0.0f,
+                               std::min(r.width(), r.height()) / 2.0f);
+  return 2.0f * (r.width() - 2.0f * rad) + 2.0f * (r.height() - 2.0f * rad) +
+         6.28318530718f * rad;
+}
+
+// 一圈上第 s 处的亮度 0..1：两个方向的 GlowWave 叠加，再拉开对比
+// （smoothstep 0.2..0.9）。白光色标和光粒共用这一个值。
+float GlowBrightness(float s, float t) {
+  const float raw = std::clamp(
+      (GlowWave(s, t, 1.0f) * 0.6f + GlowWave(s, t, -1.0f) * 0.4f) / 1.1f,
+      0.0f, 1.0f);
+  return SmoothStepForGlow(raw);
+}
+
+// 光粒用的确定性随机数 0..1：同一时刻画出来永远一样（同 liuguang.js 的
+// hash 用法），不存状态。
+float GlowHash(uint32_t x) {
+  x ^= x >> 16;
+  x *= 0x7feb352dU;
+  x ^= x >> 15;
+  x *= 0x846ca68bU;
+  x ^= x >> 16;
+  return static_cast<float>(x) / 4294967296.0f;
+}
+
+// 光粒（「光都是粒子」、11-13 定稿方向「版二上加粒子特效」）：照网页四周光效
+// liuguang.js 里「沿白光漂的微粒」（motes）缩到侧栏这一圈 4px 宽的光带里——
+// 一半顺时针、一半逆时针漂（同官网两条细丝一正一反），各自生、漂、淡出，
+// 骑在同一条起伏上（浪高处往里一点、更亮）；光暗的地方跟着淡。
+// 只在光带里（离组外沿 0.75–3.5，加上点自身半径最多约 4.8），碰不到行里的
+// 图标和字（行内容离外沿至少 10）。
+// 每 16 DIP 周长一颗（一行 224×36 约 30 颗），12–64 颗封顶。浅色侧栏上
+// 白点只有落在亮处的托底上才看得出，太疏就等于没有（11-13「白色背景下几乎
+// 不可见」「粒子有点儿少」）。
+constexpr float kMoteSpacing = 16.0f;
+constexpr int kMoteMin = 12;
+constexpr int kMoteMax = 64;
+// 漂移速度 6–16 DIP/s：网页光效微粒每秒走一圈的 0.15%–0.4%，按窗口周长
+// 约 4000 DIP 折成的实际速度，侧栏这一圈不按比例缩慢。
+constexpr float kMoteSpeedMin = 6.0f;
+constexpr float kMoteSpeedRange = 10.0f;
+// 深色侧栏上光粒的颜色：亮银（同 liuguang.js 深色处 rgb(225,232,244)）。
+constexpr SkColor kMoteDarkColor = SkColorSetRGB(225, 232, 244);
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
+// 按字形把文字放到某条中线上（不按行框居中）：基线 = 中线 + 半个
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
+// 标签在不在加载（和上段标签的转圈同一个判断：TabUIHelper 的网络状态）。
+tabs::TabNetworkState NetworkStateOf(tabs::TabInterface* tab) {
+  TabUIHelper* helper = tab ? TabUIHelper::From(tab) : nullptr;
+  return helper ? helper->GetTabNetworkState() : tabs::TabNetworkState::kNone;
+}
+
+bool TabIsLoading(tabs::TabInterface* tab) {
+  const tabs::TabNetworkState state = NetworkStateOf(tab);
+  return state == tabs::TabNetworkState::kWaiting ||
+         state == tabs::TabNetworkState::kLoading;
+}
+
+}  // namespace
+
+// ---------------------------------------------------------------------------
+// 一行：组合页行（一个 AI）或标签行。都是可点、可聚焦的按钮。
+
+class WindriseAiSectionView::Row : public views::Button,
+                                   public views::ContextMenuController {
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
+    set_context_menu_controller(this);
+  }
+  Row(const Row&) = delete;
+  Row& operator=(const Row&) = delete;
+  ~Row() override = default;
+
+  void SetData(const RowData& data) {
+    const bool tab_changed =
+        data.is_header != data_.is_header || !(data.tab == data_.tab);
+    data_ = data;
+    if (tab_changed) {
+      SubscribeAlert();
+    }
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
+  // 过渡用：这一行离可视区下沿多高（起点 / 终点）。可视区下沿在屏幕上不动
+  // （这一段贴着左下卡片），所以用「离下沿多高」插值，段高一边长、行一边走，
+  // 两者用同一条进度，不会错开。
+  int start_from_bottom() const { return start_from_bottom_; }
+  int final_from_bottom() const { return final_from_bottom_; }
+  void SetFromBottom(int start, int end) {
+    start_from_bottom_ = start;
+    final_from_bottom_ = end;
+  }
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
+      // 右边：标签数 + 箭头。标签在标题下面展开（照设计稿）：收起着箭头朝下，
+      // 展开了朝上。
+      const SkColor icon_color = text_secondary;
+      const gfx::ImageSkia chevron = gfx::CreateVectorIcon(
+          data_.expanded ? vector_icons::kKeyboardArrowUpIcon
+                         : vector_icons::kKeyboardArrowDownIcon,
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
+      // 以外的部分用段标题色。放不下整句时只放名字。
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
+    // 标签行：图标 + 标题（+ 悬停 / 当前时的「×」）。右边的位置照上段标签
+    // （tab_view_vertical_layout.cc）：「×」在最右；声音 / 录制等提示在它左边
+    // 隔 4，没有「×」时提示就占最右那一格。
+    int slot_right = width() - inset_right;
+    if (ShowsClose()) {
+      const gfx::ImageSkia close = gfx::CreateVectorIcon(
+          vector_icons::kCloseSmallIcon, kIconSize, text_secondary);
+      text_right = slot_right - kIconSize;
+      canvas->DrawImageInt(close, text_right, center_y - kIconSize / 2);
+      text_right -= kIconTitleGap;
+      slot_right -= kIconSize + kIndicatorGap;
+    }
+    if (const std::optional<tabs::TabAlert> alert = AlertToShow()) {
+      text_right = slot_right - kIconSize;
+      PaintAlert(canvas, *alert, text_right, center_y);
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
+  void ShowContextMenuForViewImpl(
+      views::View* source,
+      const gfx::Point& point,
+      ui::mojom::MenuSourceType source_type) override {
+    if (!data_.is_header && Tab()) {
+      // 复用上段的控制器、TabMenuModel 和菜单期间的保持当前收起态锁。
+      section_->show_tab_context_menu_.Run(data_.tab, point, source_type);
+    }
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
+    // 收起态那一格只放一个东西（同上段标签收起时居中的那一个）：有提示时画
+    // 提示图标，其次加载时画转圈，都没有才画网站图标。展开态提示在右边，
+    // 这一格只看加载。
+    if (section_->IsRail()) {
+      if (const std::optional<tabs::TabAlert> alert = AlertToShow()) {
+        PaintAlert(canvas, *alert, x, center_y);
+        return;
+      }
+    }
+    const tabs::TabNetworkState network = NetworkStateOf(tab);
+    if (network == tabs::TabNetworkState::kWaiting ||
+        network == tabs::TabNetworkState::kLoading) {
+      // 和上段标签同一个转圈（gfx::PaintThrobberSpinning、同一组颜色）；
+      // 节拍由本段的合成器帧时钟推（OnAnimationStep 里重画在转的行）。
+      gfx::PaintThrobberSpinning(
+          canvas, gfx::Rect(x, y, kIconSize, kIconSize),
+          colors->GetColor(network == tabs::TabNetworkState::kWaiting
+                               ? kColorTabThrobberPreconnect
+                               : kColorTabThrobber),
+          section_->ThrobberElapsed());
+      return;
+    }
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
+  std::optional<tabs::TabAlert> AlertToShow() const {
+    if (data_.is_header) {
+      return std::nullopt;
+    }
+    tabs::TabInterface* tab = Tab();
+    const tabs::TabAlertController* alerts =
+        tab ? tabs::TabAlertController::From(tab) : nullptr;
+    return alerts ? alerts->GetAlertToShow() : std::nullopt;
+  }
+
+  // 提示图标：和上段标签的提示同一套图标和颜色（tab_alert_icon.h）。
+  void PaintAlert(gfx::Canvas* canvas,
+                  tabs::TabAlert alert,
+                  int x,
+                  int center_y) {
+    const bool frame_active = GetWidget() && GetWidget()->ShouldPaintAsActive();
+    const SkColor color = GetColorProvider()->GetColor(
+        tabs::GetAlertIndicatorColor(alert, data_.active, frame_active));
+    canvas->DrawImageInt(
+        gfx::CreateVectorIcon(tabs::GetAlertIcon(alert), kIconSize, color), x,
+        center_y - kIconSize / 2);
+  }
+
+  // 提示变了（开始放声音、开始录屏……）就重画。订阅跟着这一行代理的标签换。
+  void SubscribeAlert() {
+    alert_subscription_ = {};
+    tabs::TabInterface* tab = data_.is_header ? nullptr : Tab();
+    tabs::TabAlertController* alerts =
+        tab ? tabs::TabAlertController::From(tab) : nullptr;
+    if (!alerts) {
+      return;
+    }
+    alert_subscription_ = alerts->AddAlertToShowChangedCallback(
+        base::BindRepeating(
+            [](Row* row, std::optional<tabs::TabAlert>) {
+              row->SchedulePaint();
+            },
+            base::Unretained(this)));
+  }
+
+  const raw_ptr<WindriseAiSectionView> section_;
+  RowData data_;
+  int start_from_bottom_ = 0;
+  int final_from_bottom_ = 0;
+  base::CallbackListSubscription alert_subscription_;
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
+        static_cast<float>(section_->indicator_transition_.value());
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
+// 「正在操作」光效：正在操作的 AI 那一组（标题行 + 展开的标签行）外沿
+// 一圈银白流光。画在可视区里自己的一层上：每帧只重画这一层，行不跟着重画；
+// 光只往组里面羽化（按组的圆角裁），不往外溢，所以贴着可视区边的那一组也不会
+// 被裁出一道硬边。
+//
+// 层次照网页四周光效（liuguang.js render 的前两层）缩到侧栏的尺度：
+//  1. 浅色侧栏上，亮处底下垫一层冷灰托底 rgb(132,143,158)（白底上白光才
+//     看得见）；暗色侧栏不垫（网页光效暗处同样不垫）。
+//  2. 白光三圈：只在起伏高处亮，亮处更宽、暗处几乎没有——一段段亮光沿边
+//     起伏，不是一圈等宽的框。
+// 亮度沿一圈按 GlowWave 两个方向叠加：四周同时都有亮处、原地起伏。
+//  3. 光粒：沿光带漂的白色小圆点（PaintMotes），浅色侧栏上是不透明纯白、
+//     淡入淡出靠变小（半透明白点落在托底上会成灰点）。
+// 不加脉冲：这是高频出现、可能亮好几分钟的状态光，克制为先。
+class WindriseAiSectionView::OperatingGlow : public views::View {
+  METADATA_HEADER(OperatingGlow, views::View)
+
+ public:
+  explicit OperatingGlow(WindriseAiSectionView* section) : section_(section) {
+    SetPaintToLayer();
+    layer()->SetFillsBoundsOpaquely(false);
+    SetCanProcessEventsWithinSubtree(false);
+    GetViewAccessibility().SetIsIgnored(true);
+  }
+  OperatingGlow(const OperatingGlow&) = delete;
+  OperatingGlow& operator=(const OperatingGlow&) = delete;
+  ~OperatingGlow() override = default;
+
+  void OnPaint(gfx::Canvas* canvas) override {
+    const ui::ColorProvider* colors = GetColorProvider();
+    if (!colors || section_->glow_fades_.empty()) {
+      return;
+    }
+    // 字是深色 = 浅色侧栏。
+    const bool light_sidebar = color_utils::IsDark(
+        colors->GetColor(kColorTabForegroundActiveFrameActive));
+    const float t = section_->GlowTime();
+    const float radius = static_cast<float>(Metric(LayoutMetric::kTabRadius));
+    for (const auto& [name, fade] : section_->glow_fades_) {
+      const float k = section_->GlowAlpha(name);
+      if (k <= 0.001f) {
+        continue;
+      }
+      std::optional<gfx::RectF> group;
+      const auto add = [&](const Row* row) {
+        if (row->data().ai_name != name) {
+          return;
+        }
+        const gfx::RectF bounds(row->bounds());
+        if (group) {
+          group->Union(bounds);
+        } else {
+          group = bounds;
+        }
+      };
+      for (const Row* row : section_->rows_) {
+        add(row);
+      }
+      for (const Row* row : section_->exiting_rows_) {
+        add(row);
+      }
+      if (group && !group->IsEmpty()) {
+        PaintGroup(canvas, *group, radius, k, t, light_sidebar);
+      }
+    }
+  }
+
+ private:
+  static void PaintGroup(gfx::Canvas* canvas,
+                         const gfx::RectF& group,
+                         float radius,
+                         float k,
+                         float t,
+                         bool light_sidebar) {
+    // 出现的头一段亮得快一点（同 liuguang kk = min(1, k * 1.4)）。
+    const float kk = std::min(1.0f, k * 1.4f);
+    std::array<float, kGlowStops> white;
+    for (size_t i = 0; i < white.size(); ++i) {
+      // s 从 0 到 1（GlowWave 对 s 以 1 为周期，首尾两个色标同色，接缝不露）。
+      // 拉开对比：亮处亮、暗处几乎没有，一圈上是一段段亮光在起伏，不是一圈
+      // 匀的线（离线连拍条里匀的那版读成了框）。
+      white[i] =
+          GlowBrightness(static_cast<float>(i) / (kGlowStops - 1), t);
+    }
+    const SkPoint center = SkPoint::Make(group.CenterPoint().x(),
+                                         group.CenterPoint().y());
+    gfx::ScopedCanvas scoped(canvas);
+    canvas->sk_canvas()->clipRRect(
+        SkRRect::MakeRectXY(gfx::RectFToSkRect(group), radius, radius),
+        /*do_anti_alias=*/true);
+    // 一圈：离组外沿 inset 处、宽 width 的描边，颜色 color，第 i 个色标的透明度
+    // alpha(i)。亮度沿一圈的分布用扫角渐变给。
+    const auto ring = [&](float inset, float width, SkColor color,
+                          auto alpha) {
+      // alpha(i)：第 i 个色标（一圈上第 i 个等弧长点）的透明度。
+      gfx::RectF r = group;
+      r.Inset(inset);
+      if (r.IsEmpty()) {
+        return;
+      }
+      const float rr = std::max(0.0f, radius - inset);
+      std::array<SkColor4f, kGlowStops> stops;
+      std::array<SkScalar, kGlowStops> positions;
+      for (size_t i = 0; i < stops.size(); ++i) {
+        stops[i] = SkColor4f::FromColor(color);
+        stops[i].fA = std::clamp(alpha(i), 0.0f, 1.0f);
+        // 色标位置 = 一圈上第 i 个等弧长点相对中心的角度（0..1），首尾固定。
+        float position = 0.0f;
+        if (i + 1 == stops.size()) {
+          position = 1.0f;
+        } else if (i > 0) {
+          const gfx::PointF p = PerimeterPoint(
+              r, rr, static_cast<float>(i) / (kGlowStops - 1));
+          float angle = std::atan2(p.y() - center.y(), p.x() - center.x());
+          if (angle < 0.0f) {
+            angle += 6.28318530718f;
+          }
+          position = angle / 6.28318530718f;
+        }
+        // 必须严格递增（凸形状上本来就递增，这里只防浮点抖动）。
+        if (i > 0) {
+          position = std::max(position, positions[i - 1] + 1e-5f);
+        }
+        positions[i] = std::min(position, 1.0f);
+      }
+      cc::PaintFlags flags;
+      flags.setAntiAlias(true);
+      flags.setStyle(cc::PaintFlags::kStroke_Style);
+      flags.setStrokeWidth(width);
+      flags.setShader(cc::PaintShader::MakeSweepGradient(
+          center.x(), center.y(), stops.data(), positions.data(), kGlowStops,
+          SkTileMode::kClamp, 0.0f, 360.0f));
+      canvas->sk_canvas()->drawRRect(
+          SkRRect::MakeRectXY(gfx::RectFToSkRect(r), rr, rr), flags);
+    };
+    if (light_sidebar) {
+      // 冷灰托底只垫在亮处底下、紧贴白光里侧、比白光宽（同 liuguang 托底跟着
+      // 白光起伏、略宽、.74 浓度的意思）：白光在 #DDE1E5 上才看得出银色。
+      ring(2.75f, 3.5f, kGlowBedColor,
+           [&](size_t i) { return 0.70f * kk * white[i]; });
+    }
+    const float tone = light_sidebar ? 1.0f : 0.85f;
+    ring(1.0f, 2.0f, SK_ColorWHITE,
+         [&](size_t i) { return tone * kk * (0.12f + 0.88f * white[i]); });
+    ring(2.0f, 1.5f, SK_ColorWHITE,
+         [&](size_t i) { return tone * kk * 0.6f * white[i]; });
+    ring(3.25f, 1.5f, SK_ColorWHITE,
+         [&](size_t i) { return tone * kk * 0.3f * white[i] * white[i]; });
+    PaintMotes(canvas, group, radius, kk, t, light_sidebar);
+  }
+
+  // 光粒，画在白光上面、同一个圆角裁切里。
+  static void PaintMotes(gfx::Canvas* canvas,
+                         const gfx::RectF& group,
+                         float radius,
+                         float kk,
+                         float t,
+                         bool light_sidebar) {
+    constexpr float kPi = 3.14159265359f;
+    const float perimeter = PerimeterLength(group, radius);
+    if (perimeter <= 0.0f) {
+      return;
+    }
+    const int count = std::clamp(static_cast<int>(perimeter / kMoteSpacing),
+                                 kMoteMin, kMoteMax);
+    cc::PaintFlags flags;
+    flags.setAntiAlias(true);
+    flags.setStyle(cc::PaintFlags::kFill_Style);
+    for (int i = 0; i < count; ++i) {
+      const uint32_t seed = 0x9E3779B9U * static_cast<uint32_t>(i + 1);
+      const float dir = (i % 2) ? -1.0f : 1.0f;
+      const float speed =
+          kMoteSpeedMin + GlowHash(seed ^ 0x51U) * kMoteSpeedRange;
+      const float life = 3.0f + GlowHash(seed ^ 0xA7U) * 3.0f;
+      // 第 n 轮：从一个新位置生出来，漂一段，淡出。
+      const float cycle = t / life + GlowHash(seed ^ 0x3CU);
+      const float n = std::floor(cycle);
+      const float age = (cycle - n) * life;
+      float s = GlowHash(seed + static_cast<uint32_t>(n) * 131U) +
+                dir * speed * age / perimeter;
+      s -= std::floor(s);
+      const float w = std::min(1.0f, GlowWave(s, t, dir));
+      const float env = std::pow(std::sin(kPi * age / life), 1.2f) *
+                        (0.4f + 0.6f * w) * kk *
+                        (0.15f + 0.85f * GlowBrightness(s, t));
+      if (env <= 0.02f) {
+        continue;
+      }
+      const float depth =
+          0.75f + (0.25f + 0.75f * GlowHash(seed ^ 0xD2U)) * (0.8f + 1.95f * w);
+      gfx::RectF path = group;
+      path.Inset(depth);
+      if (path.IsEmpty()) {
+        continue;
+      }
+      const gfx::PointF p =
+          PerimeterPoint(path, std::max(0.0f, radius - depth), s);
+      const float base_radius = 0.55f + 0.4f * GlowHash(seed ^ 0x6BU);
+      float r = base_radius;
+      SkColor4f color = SkColor4f::FromColor(SK_ColorWHITE);
+      if (light_sidebar) {
+        r = base_radius * 1.35f * std::sqrt(std::min(1.0f, env));
+        if (r < 0.45f) {
+          continue;
+        }
+      } else {
+        color = SkColor4f::FromColor(kMoteDarkColor);
+        color.fA = std::min(1.0f, 0.75f * env);
+      }
+      flags.setColor(color);
+      canvas->sk_canvas()->drawOval(
+          SkRect::MakeLTRB(p.x() - r, p.y() - r, p.x() + r, p.y() + r), flags);
+    }
+  }
+
+  const raw_ptr<WindriseAiSectionView> section_;
+};
+
+BEGIN_METADATA(WindriseAiSectionView, OperatingGlow)
+END_METADATA
+
+float WindriseAiSectionView::GlowFade::ValueAt(base::TimeTicks now) const {
+  if (DoneAt(now)) {
+    return to;
+  }
+  if (now <= start) {
+    return from;
+  }
+  const double progress = (now - start) / duration;
+  return from + (to - from) * static_cast<float>(gfx::Tween::CalculateValue(
+                                  gfx::Tween::WINDRISE_EASE_OUT, progress));
+}
+
+bool WindriseAiSectionView::GlowFade::DoneAt(base::TimeTicks now) const {
+  return duration.is_zero() || now >= start + duration;
+}
+
+// ---------------------------------------------------------------------------
+
+WindriseAiSectionView::Transition::Transition(views::View* host,
+                                              gfx::AnimationDelegate* owner,
+                                              base::TimeDelta duration)
+    : views::AnimationDelegateViews(host),
+      owner_(owner),
+      animation_(gfx::Animation::RichAnimationDuration(duration),
+                 kTransitionFrameRate,
+                 this) {}
+
+WindriseAiSectionView::Transition::~Transition() = default;
+
+void WindriseAiSectionView::Transition::AnimationProgressed(
+    const gfx::Animation* animation) {
+  owner_->AnimationProgressed(animation);
+}
+
+void WindriseAiSectionView::Transition::AnimationEnded(
+    const gfx::Animation* animation) {
+  owner_->AnimationEnded(animation);
+}
+
+void WindriseAiSectionView::Transition::SetDuration(base::TimeDelta duration) {
+  animation_.SetDuration(gfx::Animation::RichAnimationDuration(duration));
+}
+
+double WindriseAiSectionView::Transition::value() const {
+  if (!animation_.is_animating()) {
+    return to_;
+  }
+  return from_ + (to_ - from_) * gfx::Tween::CalculateValue(
+                                     gfx::Tween::WINDRISE_EASE_OUT,
+                                     animation_.GetCurrentValue());
+}
+
+void WindriseAiSectionView::Transition::AnimateTo(double target) {
+  const double now = value();
+  if (target == to_ && (animation_.is_animating() || now == target)) {
+    return;
+  }
+  Restart(now, target);
+}
+
+void WindriseAiSectionView::Transition::Restart(double from, double to) {
+  from_ = from;
+  to_ = to;
+  // Stop 不跳到终点（只发 AnimationCanceled），再从 0 开始走这一次。
+  animation_.Stop();
+  animation_.Start();
+}
+
+// ---------------------------------------------------------------------------
+
+WindriseAiSectionView::WindriseAiSectionView(
+    BrowserWindowInterface* browser,
+    TabStripModel* tab_strip_model,
+    PrefService* prefs,
+    ShowTabContextMenuCallback show_tab_context_menu)
+    : browser_(browser),
+      tab_strip_model_(tab_strip_model),
+      prefs_(prefs),
+      show_tab_context_menu_(std::move(show_tab_context_menu)),
+      height_transition_(this, this, kHeightDuration),
+      splitter_transition_(this, this, kSplitterFadeDuration),
+      grip_transition_(this, this, kGripDuration),
+      indicator_transition_(this, this, kIndicatorInDuration) {
+  viewport_ = AddChildView(std::make_unique<views::View>());
+  viewport_->SetPaintToLayer();
+  viewport_->layer()->SetFillsBoundsOpaquely(false);
+  viewport_->layer()->SetMasksToBounds(true);
+  indicator_ =
+      viewport_->AddChildView(std::make_unique<ScrollIndicator>(this));
+  glow_ = viewport_->AddChildView(std::make_unique<OperatingGlow>(this));
+  clock_origin_ = base::TimeTicks::Now();
+  // 光效可能一连亮好几分钟（AI 一直在操作），关掉「观察者活跃太久」的检查。
+  set_check_active_duration(false);
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
+  if (frame_compositor_) {
+    frame_compositor_->RemoveAnimationObserver(this);
+    frame_compositor_ = nullptr;
+  }
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
+  // 碰到时：拖动比例设定 AI 段的最大高度。上段用不满它那一份时，多出来的给下段，
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
+  if (height_transition_.is_animating()) {
+    // 从过渡开始时眼前的高走向「这一刻的」目标：过渡途中窗口变高、内容又变，
+    // 目标跟着变，不会走到一半跳一下。
+    return gfx::Size(0, gfx::Tween::IntValueBetween(
+                            height_transition_.value(), height_from_, target));
+  }
+  return gfx::Size(0, target);
+}
+
+void WindriseAiSectionView::StartHeightTransition(bool capture_row_starts) {
+  if (capture_row_starts) {
+    // 每一行也从眼前的位置接着走（例如松开分隔条时上一次展开还没走完）。
+    for (Row* row : rows_) {
+      row->SetFromBottom(DisplayedFromBottom(row), row->final_from_bottom());
+    }
+    for (Row* row : exiting_rows_) {
+      row->SetFromBottom(DisplayedFromBottom(row), row->final_from_bottom());
+    }
+  }
+  if (!GetWidget() || !IsDrawn()) {
+    FinishRowTransition();
+    PreferredSizeChanged();
+    return;
+  }
+  // 从眼前的高接着走（可打断：上一次过渡没走完也从当前值出发）。
+  height_from_ = height();
+  row_transition_ = true;
+  height_transition_.Restart(0.0, 1.0);
+  PreferredSizeChanged();
+  InvalidateLayout();
+}
+
+void WindriseAiSectionView::FinishRowTransition() {
+  row_transition_ = false;
+  for (Row* row : exiting_rows_) {
+    viewport_->RemoveChildViewT(row);
+  }
+  exiting_rows_.clear();
+  for (Row* row : rows_) {
+    row->SetFromBottom(row->final_from_bottom(), row->final_from_bottom());
+  }
+  InvalidateLayout();
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
+  // 内容贴着可视区下沿（最后一行在最下面）；scroll_offset_ 是藏在下沿以下
+  // 的高度。过渡中每一行按同一条进度从起点走到终点。
+  const int row_height = RowHeight();
+  const double progress =
+      row_transition_ ? height_transition_.value() : 1.0;
+  const auto place = [&](Row* row) {
+    const int from_bottom = gfx::Tween::IntValueBetween(
+        progress, row->start_from_bottom(), row->final_from_bottom());
+    row->SetBounds(0,
+                   viewport_->height() + scroll_offset_ - from_bottom -
+                       row_height,
+                   width(), row_height);
+  };
+  for (Row* row : rows_) {
+    place(row);
+  }
+  for (Row* row : exiting_rows_) {
+    place(row);
+  }
+  constexpr int kIndicatorWidth = 4;
+  indicator_->SetBounds(std::max(0, width() - kIndicatorWidth - 2), 0,
+                        kIndicatorWidth, viewport_->height());
+  // 光效那一层盖满可视区；行动了（过渡、滚动）它也要跟着重画。
+  glow_->SetBoundsRect(viewport_->GetLocalBounds());
+  if (!glow_fades_.empty()) {
+    glow_->SchedulePaint();
+  }
+  UpdateSplitterVisibility();
+}
+
+void WindriseAiSectionView::UpdateSplitterVisibility() {
+  splitter_transition_.AnimateTo(ComputeAllocation().colliding ? 1.0 : 0.0);
+  SchedulePaint();
+}
+
+void WindriseAiSectionView::OnPaint(gfx::Canvas* canvas) {
+  views::View::OnPaint(canvas);
+  const float splitter_opacity =
+      static_cast<float>(splitter_transition_.value());
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
+           static_cast<float>(grip_transition_.value())) *
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
+  glow_->SchedulePaint();
+}
+
+void WindriseAiSectionView::AddedToWidget() {
+  views::View::AddedToWidget();
+  UpdateFrameClock();
+}
+
+void WindriseAiSectionView::RemovedFromWidget() {
+  if (frame_compositor_) {
+    frame_compositor_->RemoveAnimationObserver(this);
+    frame_compositor_ = nullptr;
+  }
+  views::View::RemovedFromWidget();
+}
+
+// --- 「正在操作」光效与帧时钟 ----------------------------------------------
+
+void WindriseAiSectionView::SyncGlowTargets() {
+  const base::TimeTicks now = base::TimeTicks::Now();
+  const bool reduce_motion = gfx::Animation::PrefersReducedMotion();
+  std::set<std::u16string> present;
+  std::set<std::u16string> operating;
+  for (const RowData& row : row_data_) {
+    if (!row.is_header) {
+      continue;
+    }
+    present.insert(row.ai_name);
+    if (row.operating) {
+      operating.insert(row.ai_name);
+    }
+  }
+  // 这个 AI 那一组整个没了：光没地方画，直接拿掉。
+  std::erase_if(glow_fades_, [&](const auto& entry) {
+    return !present.contains(entry.first);
+  });
+  for (const std::u16string& name : present) {
+    const float target = operating.contains(name) ? 1.0f : 0.0f;
+    auto it = glow_fades_.find(name);
+    if (it == glow_fades_.end()) {
+      if (target == 0.0f) {
+        continue;
+      }
+      it = glow_fades_.emplace(name, GlowFade()).first;
+    }
+    GlowFade& fade = it->second;
+    if (fade.to == target) {
+      continue;
+    }
+    // 从眼前的值接着走（中途开 / 停都能打断，不跳）。
+    fade.from = fade.ValueAt(now);
+    fade.to = target;
+    fade.start = now;
+    fade.duration = reduce_motion ? base::TimeDelta()
+                                  : (target > 0.0f ? kGlowInDuration
+                                                   : kGlowOutDuration);
+  }
+  std::erase_if(glow_fades_, [&](const auto& entry) {
+    return entry.second.to == 0.0f && entry.second.DoneAt(now);
+  });
+  glow_->SchedulePaint();
+  UpdateFrameClock();
+}
+
+float WindriseAiSectionView::GlowAlpha(const std::u16string& ai_name) const {
+  const auto it = glow_fades_.find(ai_name);
+  return it == glow_fades_.end() ? 0.0f
+                                 : it->second.ValueAt(base::TimeTicks::Now());
+}
+
+float WindriseAiSectionView::GlowTime() const {
+  if (gfx::Animation::PrefersReducedMotion()) {
+    return kGlowFreezeTime;
+  }
+  // 按真实时间推进（不按帧数），60Hz、120Hz 屏上流速一样。2000 秒一循环，
+  // 免得开得很久以后 float 精度不够、起伏变成一顿一顿。
+  const double seconds = (base::TimeTicks::Now() - clock_origin_).InSecondsF();
+  return static_cast<float>(std::fmod(seconds, 2000.0));
+}
+
+base::TimeDelta WindriseAiSectionView::ThrobberElapsed() const {
+  return base::TimeTicks::Now() - clock_origin_;
+}
+
+bool WindriseAiSectionView::AnyRowLoading() const {
+  for (const RowData& row : row_data_) {
+    if (!row.is_header && TabIsLoading(row.tab.Get())) {
+      return true;
+    }
+  }
+  return false;
+}
+
+bool WindriseAiSectionView::NeedsFrames() const {
+  const base::TimeTicks now = base::TimeTicks::Now();
+  const bool reduce_motion = gfx::Animation::PrefersReducedMotion();
+  for (const auto& [name, fade] : glow_fades_) {
+    if (!fade.DoneAt(now)) {
+      return true;  // 正在淡入 / 淡出
+    }
+    if (!reduce_motion && fade.to > 0.0f) {
+      return true;  // 亮着就在流动；减少动态效果时定格，不用帧
+    }
+  }
+  return AnyRowLoading();
+}
+
+void WindriseAiSectionView::UpdateFrameClock() {
+  ui::Compositor* compositor =
+      GetWidget() ? GetWidget()->GetCompositor() : nullptr;
+  const bool want = compositor && NeedsFrames();
+  if (frame_compositor_ && (!want || frame_compositor_ != compositor)) {
+    frame_compositor_->RemoveAnimationObserver(this);
+    frame_compositor_ = nullptr;
+  }
+  if (want && !frame_compositor_) {
+    frame_compositor_ = compositor;
+    frame_compositor_->AddAnimationObserver(this);
+  }
+}
+
+void WindriseAiSectionView::OnAnimationStep(base::TimeTicks timestamp) {
+  const base::TimeTicks now = base::TimeTicks::Now();
+  std::erase_if(glow_fades_, [&](const auto& entry) {
+    return entry.second.to == 0.0f && entry.second.DoneAt(now);
+  });
+  glow_->SchedulePaint();
+  for (Row* row : rows_) {
+    if (!row->data().is_header && TabIsLoading(row->data().tab.Get())) {
+      row->SchedulePaint();
+    }
+  }
+  if (!NeedsFrames()) {
+    // 最后一帧已经排了重画；摘下时钟，不再空转。
+    UpdateFrameClock();
+  }
+}
+
+void WindriseAiSectionView::OnCompositingShuttingDown(
+    ui::Compositor* compositor) {
+  if (frame_compositor_ == compositor) {
+    frame_compositor_->RemoveAnimationObserver(this);
+    frame_compositor_ = nullptr;
+  }
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
+  // AiState.sections 按 AI 第一次出现的先后从上往下排，整段内容贴着下沿。
+  // 每个 AI：标题行（「某某正在操作」）在上，展开了它的标签在标题下面依次排
+  // （设计稿 site-zh-ai）。
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
+    header.operating = ai_state->IsOperating(section.name);
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
+namespace {
+
+bool SameRowKey(const WindriseAiSectionView::RowKeyView& a,
+                const WindriseAiSectionView::RowKeyView& b) {
+  return a.is_header == b.is_header && a.ai_name == b.ai_name && a.tab == b.tab;
+}
+
+}  // namespace
+
+int WindriseAiSectionView::DisplayedFromBottom(const Row* row) const {
+  // 按过渡进度算，不看 bounds：两次重建之间还没排版时也取得对。
+  const double progress =
+      row_transition_ ? height_transition_.value() : 1.0;
+  return gfx::Tween::IntValueBetween(progress, row->start_from_bottom(),
+                                     row->final_from_bottom());
+}
+
+void WindriseAiSectionView::Rebuild(bool animate_height, bool from_toggle) {
+  std::vector<RowData> rows = CollectRows();
+  const bool was_empty = row_data_.empty();
+
+  bool same_shape = rows.size() == row_data_.size();
+  for (size_t i = 0; same_shape && i < rows.size(); ++i) {
+    same_shape = SameRowKey(KeyOf(rows[i]), KeyOf(row_data_[i]));
+  }
+  row_data_ = std::move(rows);
+  SyncGlowTargets();
+
+  if (same_shape) {
+    // 只有内容变了（当前标签、正在操作、数字）：原地更新，悬停状态不丢。
+    for (size_t i = 0; i < rows_.size(); ++i) {
+      rows_[i]->SetData(row_data_[i]);
+    }
+    return;
+  }
+
+  const bool can_animate =
+      animate_height && !was_empty && GetWidget() && IsDrawn();
+
+  // 1. 旧行（从上往下）和它们眼前离下沿多高。上一次过渡还没走完、正在退场的
+  //    行直接拿掉（再点一次展开时，同一个标签会建新行，从它的标题下面接着
+  //    出来）。
+  for (Row* row : exiting_rows_) {
+    viewport_->RemoveChildViewT(row);
+  }
+  exiting_rows_.clear();
+  std::vector<Row*> old_rows;
+  std::vector<int> old_fb;
+  for (Row* row : rows_) {
+    old_rows.push_back(row);
+    old_fb.push_back(DisplayedFromBottom(row));
+  }
+  rows_.clear();
+
+  // 2. 新行：同一个 AI 的标题、同一个标签沿用原来那一行（位置、悬停都接着）。
+  std::vector<int> reused_old_index(row_data_.size(), -1);
+  std::vector<bool> old_reused(old_rows.size(), false);
+  for (size_t i = 0; i < row_data_.size(); ++i) {
+    Row* row = nullptr;
+    for (size_t j = 0; j < old_rows.size(); ++j) {
+      if (!old_reused[j] &&
+          SameRowKey(KeyOf(old_rows[j]->data()), KeyOf(row_data_[i]))) {
+        row = old_rows[j];
+        old_reused[j] = true;
+        reused_old_index[i] = static_cast<int>(j);
+        break;
+      }
+    }
+    if (!row) {
+      row = viewport_->AddChildView(std::make_unique<Row>(this));
+    }
+    row->SetData(row_data_[i]);
+    rows_.push_back(row);
+  }
+
+  // 3. 终点：最后一行贴着下沿，往上依次累加。
+  const int step = RowHeight() + RowGap();
+  std::vector<int> final_fb(rows_.size());
+  for (size_t i = 0; i < rows_.size(); ++i) {
+    final_fb[i] = static_cast<int>(rows_.size() - 1 - i) * step;
+  }
+
+  // 4. 起点：沿用的行从眼前的位置出发；新出现的行跟着它上面最近一个沿用的
+  //    行一起走（展开时，标签挂在标题下面，从下沿底下跟着标题一起升上来）；
+  //    上面没有就跟下面最近的；都没有就原地出现（整段第一次出现时由段高从
+  //    下沿长出来露出它们）。
+  for (size_t i = 0; i < rows_.size(); ++i) {
+    int start = final_fb[i];
+    if (can_animate) {
+      if (reused_old_index[i] >= 0) {
+        start = old_fb[reused_old_index[i]];
+      } else {
+        bool anchored = false;
+        for (int k = static_cast<int>(i) - 1; k >= 0 && !anchored; --k) {
+          if (reused_old_index[k] >= 0) {
+            start = final_fb[i] + (old_fb[reused_old_index[k]] - final_fb[k]);
+            anchored = true;
+          }
+        }
+        for (size_t k = i + 1; k < rows_.size() && !anchored; ++k) {
+          if (reused_old_index[k] >= 0) {
+            start = final_fb[i] + (old_fb[reused_old_index[k]] - final_fb[k]);
+            anchored = true;
+          }
+        }
+      }
+    }
+    rows_[i]->SetFromBottom(start, final_fb[i]);
+  }
+
+  // 5. 不再需要的旧行：点组合页收起时留着退场——跟着它上面最近一个还在的
+  //    行一起走（标签跟着标题原路沉到下沿底下），过渡结束再拿掉。别的原因
+  //    （标签关了、AI 清了）直接拿掉，其余行补位过去。
+  for (size_t j = 0; j < old_rows.size(); ++j) {
+    if (old_reused[j]) {
+      continue;
+    }
+    Row* row = old_rows[j];
+    bool anchored = false;
+    int end_fb = old_fb[j];
+    if (can_animate && from_toggle) {
+      for (int k = static_cast<int>(j) - 1; k >= 0 && !anchored; --k) {
+        if (old_reused[k]) {
+          end_fb = old_fb[j] + (old_rows[k]->final_from_bottom() - old_fb[k]);
+          anchored = true;
+        }
+      }
+      for (size_t k = j + 1; k < old_rows.size() && !anchored; ++k) {
+        if (old_reused[k]) {
+          end_fb = old_fb[j] + (old_rows[k]->final_from_bottom() - old_fb[k]);
+          anchored = true;
+        }
+      }
+    }
+    if (!anchored) {
+      viewport_->RemoveChildViewT(row);
+      continue;
+    }
+    row->SetFromBottom(old_fb[j], end_fb);
+    row->SetEnabled(false);
+    exiting_rows_.push_back(row);
+  }
+
+  // 滚动细条永远在行的上面。
+  viewport_->ReorderChildView(indicator_, viewport_->children().size());
+
+  if (can_animate || (animate_height && was_empty)) {
+    // 段高和每一行用同一条进度：标题贴着这一段的上沿被平滑推上去 / 收回来。
+    StartHeightTransition(/*capture_row_starts=*/false);
+  } else {
+    FinishRowTransition();
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
+  // 开始 / 停止加载：转圈要不要帧跟着变。
+  UpdateFrameClock();
+}
+
+void WindriseAiSectionView::OnAiStateChanged() {
+  Rebuild(/*animate_height=*/true);
+}
+
+void WindriseAiSectionView::ToggleExpanded(const std::u16string& ai_name) {
+  bool& expanded = expanded_[ai_name];
+  expanded = !expanded;
+  Rebuild(/*animate_height=*/true, /*from_toggle=*/true);
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
+  if (indicator_transition_.target() < 1.0) {
+    indicator_transition_.SetDuration(kIndicatorInDuration);
+    indicator_transition_.AnimateTo(1.0);
+  }
+  indicator_->SchedulePaint();
+  indicator_hide_timer_.Start(
+      FROM_HERE, kIndicatorHold,
+      base::BindOnce(
+          [](WindriseAiSectionView* view) {
+            view->indicator_transition_.SetDuration(kIndicatorOutDuration);
+            view->indicator_transition_.AnimateTo(0.0);
+          },
+          base::Unretained(this)));
+}
+
+bool WindriseAiSectionView::OnMouseWheel(const ui::MouseWheelEvent& event) {
+  if (MaxScrollOffset() <= 0 || event.y_offset() == 0) {
+    return false;
+  }
+  // 和上段同一个方向：滚轮往下，内容往上走，露出下沿
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
+  grip_transition_.AnimateTo(hovered || pressed_in_splitter_ ? 1.0 : 0.0);
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
+  // 抓住的是眼前的高：段高立刻停在当前值（drag_height_ 盖过过渡），从这里
+  // 跟手；行的过渡照走完，不跳。
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
+    StartHeightTransition(/*capture_row_starts=*/true);
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
+  if (animation == height_transition_.animation()) {
+    PreferredSizeChanged();
+    InvalidateLayout();
+  } else if (animation == indicator_transition_.animation()) {
+    indicator_->SchedulePaint();
+  } else {
+    SchedulePaint();
+  }
+}
+
+void WindriseAiSectionView::AnimationEnded(const gfx::Animation* animation) {
+  AnimationProgressed(animation);
+  if (animation == height_transition_.animation()) {
+    FinishRowTransition();
+  }
+}
+
+BEGIN_METADATA(WindriseAiSectionView)
+END_METADATA
