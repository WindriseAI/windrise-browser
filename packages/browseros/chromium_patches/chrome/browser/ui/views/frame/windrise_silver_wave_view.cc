diff --git a/chrome/browser/ui/views/frame/windrise_silver_wave_view.cc b/chrome/browser/ui/views/frame/windrise_silver_wave_view.cc
new file mode 100644
index 0000000000..23695543cc
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_silver_wave_view.cc
@@ -0,0 +1,216 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/frame/windrise_silver_wave_view.h"
+
+#include <algorithm>
+#include <utility>
+#include <vector>
+
+#include "base/functional/bind.h"
+#include "cc/paint/paint_flags.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
+#include "chrome/browser/glic/resources/grit/glic_browser_resources.h"
+#include "third_party/skia/include/core/SkColor.h"
+#include "third_party/skia/include/core/SkM44.h"
+#include "third_party/skia/include/core/SkRRect.h"
+#include "third_party/skia/include/core/SkString.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/color/color_provider.h"
+#include "ui/compositor/layer.h"
+#include "ui/gfx/animation/animation.h"
+#include "ui/gfx/canvas.h"
+#include "ui/gfx/geometry/skia_conversions.h"
+
+namespace {
+
+using browseros::windrise::LayoutMetric;
+using browseros::windrise::ThemeToken;
+
+int Metric(LayoutMetric metric) {
+  return browseros::windrise::GetLayoutMetrics().Get(metric);
+}
+
+SkColor ThemeColor(ThemeToken token) {
+  return browseros::windrise::GetLightColors().Get(token);
+}
+
+SkV4 ToFloat4(SkColor color) {
+  return SkV4{SkColorGetR(color) / 255.0f, SkColorGetG(color) / 255.0f,
+              SkColorGetB(color) / 255.0f, SkColorGetA(color) / 255.0f};
+}
+
+}  // namespace
+
+WindriseSilverWaveView::WindriseSilverWaveView(Profile* profile)
+    : glic::AnimatedEffectView(profile, /*tester=*/nullptr) {
+  SetVisible(false);
+  // 光效层不接任何输入（同 glic 的边框）。
+  SetCanProcessEventsWithinSubtree(false);
+  SetPaintToLayer();
+  layer()->SetFillsBoundsOpaquely(false);
+}
+
+WindriseSilverWaveView::~WindriseSilverWaveView() = default;
+
+// static
+int WindriseSilverWaveView::GetSpread() {
+  return std::max(0, Metric(LayoutMetric::kSilverWaveSpread));
+}
+
+void WindriseSilverWaveView::SetShowing(bool showing) {
+  if (showing == IsShowing()) {
+    return;
+  }
+  if (showing) {
+    // 还没进 widget（没有 compositor）时 Show() 会上报一次无崩溃转储；先记下
+    // 想显示，等 AddedToWidget() 再开。
+    if (!parent() || !GetWidget()) {
+      wants_showing_ = true;
+      return;
+    }
+    wants_showing_ = false;
+    Show();
+  } else {
+    wants_showing_ = false;
+    StartRampingDown();
+  }
+}
+
+void WindriseSilverWaveView::SetCornerRadii(
+    const gfx::RoundedCornersF& radii) {
+  corner_radii_ = radii;
+  // 本层自己按距离场画圆角，layer 不再裁一次。
+  corner_radius_ = gfx::RoundedCornersF();
+  if (layer()) {
+    layer()->SetRoundedCornerRadius(corner_radius_);
+  }
+  SchedulePaint();
+}
+
+void WindriseSilverWaveView::AddedToWidget() {
+  glic::AnimatedEffectView::AddedToWidget();
+  // Show() 在还没有 compositor 的时候只是把层画出来、不挂观察者；进 widget 之后
+  // 补一次，光效才真的开始走。
+  if (wants_showing_ || (IsShowing() && !compositor_)) {
+    wants_showing_ = false;
+    Show();
+  }
+}
+
+void WindriseSilverWaveView::OnAnimationStep(base::TimeTicks timestamp) {
+  glic::AnimatedEffectView::OnAnimationStep(timestamp);
+  // 这是一直在跑的光效；不复位观察者的计时会每 60 秒刷一条「active too long」。
+  ResetIfActive();
+}
+
+bool WindriseSilverWaveView::IsCycleDone(base::TimeTicks timestamp) {
+  // 只要 AI 还在操作就一直流动；收尾由 StartRampingDown() 控制。
+  // 例外：GPU 不可用 / 减少动态效果时画的是静止边框，不需要每帧重绘，
+  // 起亮完成后就让合成器观察者退出（软件光栅的机器上尤其要省这个）。
+  return skip_animation_cycle_;
+}
+
+base::TimeDelta WindriseSilverWaveView::GetTotalDuration() const {
+  // 开场那一道沿边跑一圈的时长；之后 u_intro 停在 1。
+  return base::Milliseconds(Metric(LayoutMetric::kSilverWaveIntroMs));
+}
+
+int WindriseSilverWaveView::GetShaderResourceId(bool simplified) const {
+  // 简化模式也在 DrawEffect 里画静止银边，这里给同一份资源即可。
+  return IDR_WINDRISE_SILVER_WAVE_SHADER;
+}
+
+base::TimeDelta WindriseSilverWaveView::GetRampUpDuration() const {
+  return base::Milliseconds(Metric(LayoutMetric::kSilverWaveFadeInMs));
+}
+
+base::TimeDelta WindriseSilverWaveView::GetRampDownDuration() const {
+  return base::Milliseconds(Metric(LayoutMetric::kSilverWaveFadeOutMs));
+}
+
+gfx::Rect WindriseSilverWaveView::ContentsRectInView() const {
+  const int spread = GetSpread();
+  return gfx::Rect(spread, spread, std::max(0, width() - 2 * spread),
+                   std::max(0, height() - 2 * spread));
+}
+
+void WindriseSilverWaveView::PopulateShaderUniforms(
+    std::vector<cc::PaintShader::FloatUniform>& float_uniforms,
+    std::vector<cc::PaintShader::Float2Uniform>& float2_uniforms,
+    std::vector<cc::PaintShader::Float4Uniform>& float4_uniforms,
+    std::vector<cc::PaintShader::IntUniform>& int_uniforms) const {
+  const float scale = layer() ? layer()->device_scale_factor() : 1.0f;
+  const gfx::Rect contents = ContentsRectInView();
+  const gfx::RectF contents_scaled =
+      gfx::ScaleRect(gfx::RectF(contents), scale, scale);
+  const float radius_top = std::max(0.0f, corner_radii_.upper_left()) * scale;
+  const float radius_bottom =
+      std::max(0.0f, corner_radii_.lower_right()) * scale;
+
+  // 整体强度由 layer opacity 控制（这里恒 1，避免二次衰减）。
+  float_uniforms.push_back({SkString("u_k"), 1.0f});
+  float_uniforms.push_back({SkString("u_time"), GetEffectTime()});
+  float_uniforms.push_back(
+      {SkString("u_intro"), skip_animation_cycle_ ? 1.0f : GetEffectProgress(
+                                                            base::TimeTicks::Now())});
+  float_uniforms.push_back({SkString("u_scale"), scale});
+  float_uniforms.push_back(
+      {SkString("u_still"),
+       gfx::Animation::PrefersReducedMotion() ? 1.0f : 0.0f});
+
+  float2_uniforms.push_back(
+      {SkString("u_resolution"),
+       SkV2{width() * scale, height() * scale}});
+  float4_uniforms.push_back(
+      {SkString("u_rect"),
+       SkV4{contents_scaled.x(), contents_scaled.y(), contents_scaled.width(),
+            contents_scaled.height()}});
+  float4_uniforms.push_back(
+      {SkString("u_radii"),
+       SkV4{radius_top, radius_top, radius_bottom, radius_bottom}});
+
+  float4_uniforms.push_back(
+      {SkString("u_silver_wash"), ToFloat4(ThemeColor(ThemeToken::kSilverWaveWash))});
+  float4_uniforms.push_back(
+      {SkString("u_silver_soft_deep"),
+       ToFloat4(ThemeColor(ThemeToken::kSilverWaveSoftDeep))});
+  float4_uniforms.push_back(
+      {SkString("u_silver_soft_bright"),
+       ToFloat4(ThemeColor(ThemeToken::kSilverWaveSoftBright))});
+  float4_uniforms.push_back(
+      {SkString("u_silver_core_deep"),
+       ToFloat4(ThemeColor(ThemeToken::kSilverWaveCoreDeep))});
+  float4_uniforms.push_back(
+      {SkString("u_silver_core_bright"),
+       ToFloat4(ThemeColor(ThemeToken::kSilverWaveCoreBright))});
+}
+
+void WindriseSilverWaveView::DrawEffect(gfx::Canvas* canvas,
+                                        const cc::PaintFlags& flags) {
+  const gfx::Rect contents = ContentsRectInView();
+  if (contents.IsEmpty()) {
+    return;
+  }
+  if (ForceSimplifiedShader() || !flags.getShader()) {
+    // GPU 不可用 / 减少动态效果 / 着色器没编出来：静止银边。
+    const float radius =
+        std::max(0.0f, corner_radii_.upper_left());
+    cc::PaintFlags rim;
+    rim.setAntiAlias(true);
+    rim.setStyle(cc::PaintFlags::kStroke_Style);
+    rim.setStrokeWidth(1.5f);
+    SkColor color = ThemeColor(ThemeToken::kSilverWaveSoftBright);
+    rim.setColor(SkColorSetA(
+        color, static_cast<uint8_t>(0.55f * SkColorGetA(color))));
+    SkRect rect = gfx::RectToSkRect(contents);
+    rect.inset(0.75f, 0.75f);
+    canvas->sk_canvas()->drawRRect(SkRRect::MakeRectXY(rect, radius, radius),
+                                   rim);
+    return;
+  }
+  canvas->DrawRect(GetLocalBounds(), flags);
+}
+
+BEGIN_METADATA(WindriseSilverWaveView)
+END_METADATA
\ No newline at end of file
