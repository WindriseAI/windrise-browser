diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_view.cc b/chrome/browser/ui/views/frame/windrise_edge_light_view.cc
new file mode 100644
index 0000000000..fe3edb3012
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_view.cc
@@ -0,0 +1,925 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/frame/windrise_edge_light_view.h"
+
+#include <algorithm>
+#include <cmath>
+#include <utility>
+#include <vector>
+
+#include "base/functional/bind.h"
+#include "base/no_destructor.h"
+#include "base/task/bind_post_task.h"
+#include "base/timer/elapsed_timer.h"
+#include "cc/paint/paint_canvas.h"
+#include "cc/paint/paint_filter.h"
+#include "cc/paint/paint_flags.h"
+#include "content/public/browser/gpu_data_manager.h"
+#include "content/public/browser/render_widget_host_view.h"
+#include "content/public/browser/web_contents.h"
+#include "third_party/skia/include/core/SkBlendMode.h"
+#include "third_party/skia/include/core/SkColor.h"
+#include "third_party/skia/include/core/SkPath.h"
+#include "third_party/skia/include/core/SkPathBuilder.h"
+#include "third_party/skia/include/core/SkRRect.h"
+#include "third_party/skia/include/core/SkRect.h"
+#include "third_party/skia/include/core/SkTileMode.h"
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/compositor/layer.h"
+#include "ui/gfx/animation/animation.h"
+#include "ui/gfx/canvas.h"
+#include "ui/gfx/geometry/cubic_bezier.h"
+#include "ui/gfx/geometry/rect.h"
+#include "ui/gfx/geometry/size.h"
+#include "ui/views/widget/widget.h"
+
+namespace {
+
+using browseros::windrise::Clamp01;
+using browseros::windrise::EdgeLightTier;
+using browseros::windrise::EdgeLuminance;
+using browseros::windrise::EdgeParticle;
+using browseros::windrise::EdgePerimeter;
+using browseros::windrise::EdgePoint;
+using browseros::windrise::SmoothStep;
+
+constexpr float kTwoPi = 6.28318530717958647692f;
+
+// 方案：起 300ms 强缓出，收 400ms 线性；网页角过渡 300ms，
+// cubic-bezier(.23, 1, .32, 1)。
+constexpr double kRampUpSeconds = 0.300;
+constexpr double kRampDownSeconds = 0.400;
+constexpr double kLuminanceBlendSeconds = 0.300;
+constexpr base::TimeDelta kThumbnailInterval = base::Seconds(1);
+
+float Ease(float x) {
+  static const base::NoDestructor<gfx::CubicBezier> bezier(0.23, 1.0, 0.32, 1.0);
+  return static_cast<float>(bezier->Solve(Clamp01(x)));
+}
+
+float Lerp(float a, float b, float t) {
+  return a + (b - a) * t;
+}
+
+SkColor Rgba(int r, int g, int b, float alpha) {
+  return SkColorSetARGB(
+      static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f), r, g,
+      b);
+}
+
+SkColor LerpRgb(int r0, int g0, int b0, int r1, int g1, int b1, float t,
+                float alpha) {
+  return Rgba(static_cast<int>(Lerp(static_cast<float>(r0), static_cast<float>(r1), t)),
+              static_cast<int>(Lerp(static_cast<float>(g0), static_cast<float>(g1), t)),
+              static_cast<int>(Lerp(static_cast<float>(b0), static_cast<float>(b1), t)),
+              alpha);
+}
+
+// 一帧的绘制上下文：把「这一帧的数」和「画布」放在一起，各层函数共用。
+struct Ctx {
+  cc::PaintCanvas* canvas = nullptr;
+  const EdgePerimeter* perimeter = nullptr;
+  const EdgeLuminance* luminance = nullptr;
+  const EdgeLightTier* tier = nullptr;
+  float t = 0;
+  float k = 1;
+  float env = 0;
+  float wide = 1;
+  float breath = 1;
+  float edge_fade = 1;
+  float lt = 1;  // 整圈浅色程度（亮 = 1）
+  bool blur = true;
+  int level = 0;
+};
+
+int N(const Ctx& c) {
+  return static_cast<int>(c.perimeter->points().size());
+}
+
+// 光效层在某个深度（离边向里）上的一圈点；深度被走线点的上限夹住。
+std::vector<SkPoint> LoopAtDepth(const Ctx& c, const std::vector<float>& depth) {
+  const auto& pts = c.perimeter->points();
+  std::vector<SkPoint> out;
+  out.reserve(pts.size());
+  for (size_t i = 0; i < pts.size(); ++i) {
+    const float d = std::clamp(depth[i], 0.0f, pts[i].depth_cap);
+    out.push_back(SkPoint::Make(pts[i].x + pts[i].nx * d, pts[i].y + pts[i].ny * d));
+  }
+  return out;
+}
+
+// 「从前沿到边外 30px 的一条带子」：外圈 + 前沿内圈，奇偶填充得到环。
+// [begin, end] 是点下标范围（含），begin == 0 且 end == N-1 时画整圈。
+SkPath BandPath(const Ctx& c,
+                const std::vector<SkPoint>& front,
+                int begin,
+                int end) {
+  const auto& pts = c.perimeter->points();
+  SkPathBuilder b;
+  b.setFillType(SkPathFillType::kEvenOdd);
+  const bool full = begin == 0 && end >= static_cast<int>(pts.size()) - 1;
+  const float outer = browseros::windrise::kEdgeLightOuterBand;
+  if (full) {
+    for (int pass = 0; pass < 2; ++pass) {
+      for (size_t i = 0; i < pts.size(); ++i) {
+        const SkPoint p =
+            pass == 0 ? SkPoint::Make(pts[i].x - pts[i].nx * outer,
+                                      pts[i].y - pts[i].ny * outer)
+                      : front[i];
+        if (i == 0) {
+          b.moveTo(p);
+        } else {
+          b.lineTo(p);
+        }
+      }
+      b.close();
+    }
+  } else {
+    // 下标按圈取模：最后一段的 end 可以越过末点回到 0，补上首尾之间那条缝。
+    const int sz = static_cast<int>(pts.size());
+    for (int i = begin; i <= end; ++i) {
+      const EdgePoint& q = pts[i % sz];
+      const SkPoint p = SkPoint::Make(q.x - q.nx * outer, q.y - q.ny * outer);
+      if (i == begin) {
+        b.moveTo(p);
+      } else {
+        b.lineTo(p);
+      }
+    }
+    for (int i = end; i >= begin; --i) {
+      b.lineTo(front[i % sz]);
+    }
+    b.close();
+  }
+  return b.detach();
+}
+
+SkPath LoopPath(const std::vector<SkPoint>& loop) {
+  SkPathBuilder b;
+  for (size_t i = 0; i < loop.size(); ++i) {
+    if (i == 0) {
+      b.moveTo(loop[i]);
+    } else {
+      b.lineTo(loop[i]);
+    }
+  }
+  b.close();
+  return b.detach();
+}
+
+// 模糊（半径按 CSS 口径，sigma = 半径 / 2）。GPU 不可用时不模糊。
+void SetBlur(const Ctx& c, cc::PaintFlags* flags, float radius) {
+  if (!c.blur || radius < 0.3f) {
+    return;
+  }
+  const float sigma = radius * 0.5f;
+  flags->setImageFilter(sk_make_sp<cc::BlurPaintFilter>(
+      sigma, sigma, SkTileMode::kDecal, nullptr));
+}
+
+void FillBand(const Ctx& c,
+              const std::vector<float>& depth,
+              SkColor color,
+              float blur_radius) {
+  cc::PaintFlags flags;
+  flags.setAntiAlias(true);
+  flags.setStyle(cc::PaintFlags::kFill_Style);
+  flags.setColor(color);
+  SetBlur(c, &flags, blur_radius);
+  const int n = N(c);
+  c.canvas->drawPath(BandPath(c, LoopAtDepth(c, depth), 0, n - 1), flags);
+}
+
+// 一笔画的连续线（所有沿边的线都这样画，不许一段一段描）：整条线实成一条路径，
+// 另画一张「权重图」（每段一小块，透明度 = 该处亮度）模糊后用 kDstIn 乘上去。
+// `weights` 为空 = 亮度恒 1。
+void DrawContinuousLine(const Ctx& c,
+                        const std::vector<float>& depth,
+                        float width,
+                        SkColor color,
+                        const std::vector<float>& weights,
+                        float weight_blur_radius,
+                        float line_blur_radius) {
+  const std::vector<SkPoint> loop = LoopAtDepth(c, depth);
+  cc::PaintCanvas* canvas = c.canvas;
+
+  cc::PaintFlags layer_flags;
+  layer_flags.setAlphaf(SkColorGetA(color) / 255.0f);
+  SetBlur(c, &layer_flags, line_blur_radius);
+  canvas->saveLayer(layer_flags);
+
+  cc::PaintFlags stroke;
+  stroke.setAntiAlias(true);
+  stroke.setStyle(cc::PaintFlags::kStroke_Style);
+  stroke.setStrokeWidth(width);
+  stroke.setStrokeJoin(cc::PaintFlags::kRound_Join);
+  stroke.setColor(SkColorSetRGB(SkColorGetR(color), SkColorGetG(color),
+                                SkColorGetB(color)));
+  canvas->drawPath(LoopPath(loop), stroke);
+
+  if (!weights.empty()) {
+    cc::PaintFlags weight_layer;
+    weight_layer.setBlendMode(SkBlendMode::kDstIn);
+    SetBlur(c, &weight_layer, weight_blur_radius);
+    canvas->saveLayer(weight_layer);
+    const int n = static_cast<int>(loop.size());
+    constexpr int kChunk = 4;  // 4 个点一块，模糊之后看不出分块
+    cc::PaintFlags piece;
+    piece.setAntiAlias(true);
+    piece.setStyle(cc::PaintFlags::kStroke_Style);
+    piece.setStrokeWidth(width + 6.0f);
+    piece.setStrokeCap(cc::PaintFlags::kRound_Cap);
+    for (int i = 0; i < n; i += kChunk) {
+      const int j = std::min(i + kChunk, n);
+      float w = 0;
+      for (int q = i; q <= j; ++q) {
+        w += weights[q % n];
+      }
+      w /= static_cast<float>(j - i + 1);
+      piece.setColor(SkColorSetARGB(
+          static_cast<uint8_t>(Clamp01(w) * 255.0f + 0.5f), 0, 0, 0));
+      SkPathBuilder seg;
+      seg.moveTo(loop[i]);
+      for (int q = i + 1; q <= j; ++q) {
+        seg.lineTo(loop[q % n]);
+      }
+      canvas->drawPath(seg.detach(), piece);
+    }
+    canvas->restore();
+  }
+  canvas->restore();
+}
+
+void DrawDot(cc::PaintCanvas* canvas,
+             const cc::PaintFlags& flags,
+             float x,
+             float y,
+             float r) {
+  canvas->drawOval(SkRect::MakeLTRB(x - r, y - r, x + r, y + r), flags);
+}
+
+}  // namespace
+
+// ---------------------------------------------------------------------------
+// 生命周期
+// ---------------------------------------------------------------------------
+WindriseEdgeLightView::WindriseEdgeLightView() {
+  SetVisible(false);
+  // 光效层不接任何输入。
+  SetCanProcessEventsWithinSubtree(false);
+  SetPaintToLayer();
+  layer()->SetFillsBoundsOpaquely(false);
+  // 这是一直在跑的光效，不让合成器因为「活跃超过 60 秒」报失败（要在加入合成器
+  // 之前设）。
+  set_check_active_duration(false);
+
+  content::GpuDataManager* gpu = content::GpuDataManager::GetInstance();
+  gpu_blur_ok_ = gpu->IsGpuRasterizationForUIEnabled();
+  gpu_observation_.Observe(gpu);
+}
+
+WindriseEdgeLightView::~WindriseEdgeLightView() = default;
+
+void WindriseEdgeLightView::SetShowing(bool showing) {
+  wants_showing_ = showing;
+  ApplyWantedState();
+}
+
+void WindriseEdgeLightView::SetCornerProgressCallback(
+    CornerProgressCallback callback) {
+  corner_callback_ = std::move(callback);
+}
+
+void WindriseEdgeLightView::SetWebContents(content::WebContents* web_contents) {
+  web_contents_ = web_contents ? web_contents->GetWeakPtr()
+                               : base::WeakPtr<content::WebContents>();
+}
+
+void WindriseEdgeLightView::AddedToWidget() {
+  views::View::AddedToWidget();
+  // 要求显示时本层还没进 widget（没有合成器）：这里补一次。
+  ApplyWantedState();
+}
+
+void WindriseEdgeLightView::RemovedFromWidget() {
+  views::View::RemovedFromWidget();
+  StopObservingCompositor();
+}
+
+void WindriseEdgeLightView::ApplyWantedState() {
+  if (wants_showing_) {
+    if (phase_ == Phase::kHidden || phase_ == Phase::kRampingDown) {
+      if (!GetWidget() || !GetWidget()->GetCompositor()) {
+        return;
+      }
+      Start();
+    }
+  } else if (phase_ == Phase::kRampingUp || phase_ == Phase::kSteady) {
+    BeginRampDown();
+  }
+}
+
+void WindriseEdgeLightView::Start() {
+  reduced_motion_ = gfx::Animation::PrefersReducedMotion();
+  const bool from_hidden = phase_ == Phase::kHidden;
+  SetVisible(true);
+  appear_from_ = from_hidden ? 0.0f : appear_;
+  corner_from_ = corner_progress_;
+  phase_start_pending_ = true;
+  if (from_hidden) {
+    effect_start_ = base::TimeTicks();
+    effect_time_ = 0;
+    pulse_age_ = -1;
+    UpdatePerimeter();
+    thumbnail_timer_.Start(FROM_HERE, kThumbnailInterval,
+                           base::BindRepeating(&WindriseEdgeLightView::RequestThumbnail,
+                                               base::Unretained(this)));
+    RequestThumbnail();
+  }
+  if (reduced_motion_) {
+    // 减少动态效果：定格在 t = 2.35 秒，不呼吸、不脉冲、粒子不动（仍显示）。
+    phase_ = Phase::kSteady;
+    appear_ = 1.0f;
+    corner_progress_ = 1.0f;
+    effect_time_ = browseros::windrise::kEdgeLightFreezeTime;
+    NotifyCornerProgress(/*force=*/true);
+    StopObservingCompositor();
+  } else {
+    phase_ = Phase::kRampingUp;
+    ObserveCompositor();
+  }
+  SchedulePaint();
+}
+
+void WindriseEdgeLightView::BeginRampDown() {
+  if (reduced_motion_) {
+    Hide();
+    return;
+  }
+  phase_ = Phase::kRampingDown;
+  appear_from_ = appear_;
+  corner_from_ = corner_progress_;
+  phase_start_pending_ = true;
+  ObserveCompositor();
+}
+
+void WindriseEdgeLightView::Hide() {
+  phase_ = Phase::kHidden;
+  appear_ = 0.0f;
+  corner_progress_ = 0.0f;
+  pulse_age_ = -1.0f;
+  ripple_source_s_.reset();
+  luminance_ = EdgeLuminance();
+  luminance_target_ = EdgeLuminance();
+  luminance_blending_ = false;
+  thumbnail_timer_.Stop();
+  StopObservingCompositor();
+  SetVisible(false);
+  // 回到基准时一定通知一次，网页角才会真的收回去。
+  NotifyCornerProgress(/*force=*/true);
+}
+
+void WindriseEdgeLightView::ObserveCompositor() {
+  if (compositor_observation_.IsObserving()) {
+    return;
+  }
+  ui::Compositor* compositor = GetWidget() ? GetWidget()->GetCompositor() : nullptr;
+  if (!compositor) {
+    return;
+  }
+  compositor_ = compositor;
+  compositor_observation_.Observe(compositor);
+}
+
+void WindriseEdgeLightView::StopObservingCompositor() {
+  compositor_observation_.Reset();
+  compositor_ = nullptr;
+}
+
+void WindriseEdgeLightView::OnCompositingShuttingDown(
+    ui::Compositor* compositor) {
+  StopObservingCompositor();
+  if (phase_ != Phase::kHidden) {
+    Hide();
+  }
+}
+
+void WindriseEdgeLightView::OnGpuInfoUpdate() {
+  const bool ok =
+      content::GpuDataManager::GetInstance()->IsGpuRasterizationForUIEnabled();
+  if (ok != gpu_blur_ok_) {
+    gpu_blur_ok_ = ok;
+    if (phase_ != Phase::kHidden) {
+      SchedulePaint();
+    }
+  }
+}
+
+// ---------------------------------------------------------------------------
+// 每帧
+// ---------------------------------------------------------------------------
+void WindriseEdgeLightView::OnAnimationStep(base::TimeTicks timestamp) {
+  last_step_time_ = timestamp;
+  if (phase_start_pending_) {
+    phase_start_ = timestamp;
+    phase_start_pending_ = false;
+  }
+  if (effect_start_.is_null()) {
+    effect_start_ = timestamp;
+  }
+  effect_time_ = reduced_motion_
+                     ? browseros::windrise::kEdgeLightFreezeTime
+                     : static_cast<float>((timestamp - effect_start_).InSecondsF());
+
+  const double elapsed = (timestamp - phase_start_).InSecondsF();
+  switch (phase_) {
+    case Phase::kRampingUp: {
+      const float x = static_cast<float>(std::clamp(elapsed / kRampUpSeconds, 0.0, 1.0));
+      const float e = Ease(x);
+      appear_ = Lerp(appear_from_, 1.0f, e);
+      corner_progress_ = Lerp(corner_from_, 1.0f, e);
+      if (x >= 1.0f) {
+        phase_ = Phase::kSteady;
+        appear_ = 1.0f;
+        corner_progress_ = 1.0f;
+      }
+      break;
+    }
+    case Phase::kRampingDown: {
+      const float x = static_cast<float>(std::clamp(elapsed / kRampDownSeconds, 0.0, 1.0));
+      appear_ = appear_from_ * (1.0f - x);          // 线性淡出
+      corner_progress_ = corner_from_ * (1.0f - Ease(x));  // 网页角随光收回
+      if (x >= 1.0f) {
+        Hide();
+        return;
+      }
+      break;
+    }
+    case Phase::kSteady:
+      appear_ = 1.0f;
+      corner_progress_ = 1.0f;
+      break;
+    case Phase::kHidden:
+      StopObservingCompositor();
+      return;
+  }
+  NotifyCornerProgress(/*force=*/false);
+
+  if (!pulse_time_.is_null()) {
+    pulse_age_ = static_cast<float>((timestamp - pulse_time_).InSecondsF());
+    if (pulse_age_ > 0.9f) {
+      pulse_age_ = -1.0f;
+      pulse_time_ = base::TimeTicks();
+      ripple_source_s_.reset();
+    }
+  }
+  BlendLuminance(timestamp);
+
+  // 帧预算降档：60 帧 / 30 帧。网页角过渡的回调上面已经每帧发了，这里只限绘制。
+  const int level = std::max(degrade_level_, gpu_blur_ok_ ? 0 : 2);
+  const base::TimeDelta interval = level >= 2 ? base::Milliseconds(33)
+                                  : level == 1 ? base::Milliseconds(16)
+                                               : base::TimeDelta();
+  if (!interval.is_zero() && !last_paint_time_.is_null() &&
+      timestamp - last_paint_time_ < interval - base::Milliseconds(2)) {
+    return;
+  }
+  last_paint_time_ = timestamp;
+  SchedulePaint();
+}
+
+void WindriseEdgeLightView::NotifyCornerProgress(bool force) {
+  if (!force && std::fabs(corner_progress_ - last_notified_corner_) < 0.0005f) {
+    return;
+  }
+  last_notified_corner_ = corner_progress_;
+  if (corner_callback_) {
+    corner_callback_.Run(corner_progress_);
+  }
+}
+
+void WindriseEdgeLightView::Pulse(const browseros::windrise::AiPulse& pulse) {
+  if (phase_ == Phase::kHidden || phase_ == Phase::kRampingDown ||
+      reduced_motion_) {
+    return;
+  }
+  UpdatePerimeter();
+  pulse_time_ = pulse.time;
+  pulse_age_ = 0.0f;
+  ripple_source_s_.reset();
+  if (pulse.has_point && !perimeter_.empty()) {
+    const float x = std::clamp(pulse.x, 0.0f, static_cast<float>(width()));
+    const float y = std::clamp(pulse.y, 0.0f, static_cast<float>(height()));
+    ripple_source_s_ = perimeter_.NearestS(x, y);
+  }
+  SchedulePaint();
+}
+
+void WindriseEdgeLightView::SetCornerRadii(const gfx::RoundedCornersF& radii) {
+  if (radii == corner_radii_) {
+    return;
+  }
+  corner_radii_ = radii;
+  UpdatePerimeter();
+  if (phase_ != Phase::kHidden) {
+    SchedulePaint();
+  }
+}
+
+void WindriseEdgeLightView::UpdatePerimeter() {
+  const std::array<float, 4> radii = {
+      corner_radii_.upper_left(), corner_radii_.upper_right(),
+      corner_radii_.lower_right(), corner_radii_.lower_left()};
+  const gfx::SizeF size(width(), height());
+  if (!perimeter_.empty() && size == perimeter_size_ && radii == perimeter_radii_) {
+    return;
+  }
+  perimeter_size_ = size;
+  perimeter_radii_ = radii;
+  perimeter_.Build(size.width(), size.height(), radii);
+}
+
+// ---------------------------------------------------------------------------
+// 亮度取样（每秒一次，只在内存里）
+// ---------------------------------------------------------------------------
+void WindriseEdgeLightView::RequestThumbnail() {
+  if (phase_ == Phase::kHidden) {
+    return;
+  }
+  content::RenderWidgetHostView* view =
+      web_contents_ ? web_contents_->GetRenderWidgetHostView() : nullptr;
+  if (!view || width() < 8 || height() < 8) {
+    ApplyFallbackLuminance();
+    return;
+  }
+  // 一个像素约等于文字遮罩的一格（4px）。
+  const gfx::Size output(std::max(1, width() / 4), std::max(1, height() / 4));
+  auto on_result = base::BindOnce(
+      [](base::WeakPtr<WindriseEdgeLightView> self,
+         const content::CopyFromSurfaceResult& result) {
+        if (!self) {
+          return;
+        }
+        if (result.has_value()) {
+          self->OnThumbnail(result->bitmap);
+        } else {
+          self->ApplyFallbackLuminance();
+        }
+      },
+      weak_factory_.GetWeakPtr());
+  // 回调不保证在同一个序列上跑。
+  view->CopyFromSurface(gfx::Rect(), output, base::Milliseconds(500),
+                        base::BindPostTaskToCurrentDefault(std::move(on_result)));
+}
+
+void WindriseEdgeLightView::OnThumbnail(const SkBitmap& bitmap) {
+  if (phase_ == Phase::kHidden) {
+    return;
+  }
+  UpdatePerimeter();
+  EdgeLuminance target = browseros::windrise::AnalyzeThumbnail(bitmap, perimeter_);
+  if (!target.valid) {
+    ApplyFallbackLuminance();
+    return;
+  }
+  if (!luminance_.valid) {
+    luminance_ = target;
+    luminance_target_ = std::move(target);
+    luminance_blending_ = false;
+    return;
+  }
+  // 300ms 线性插值到新值；文字遮罩直接换。
+  luminance_from_ = luminance_.point_luminance;
+  overall_from_ = luminance_.overall;
+  luminance_.text_rects = target.text_rects;
+  luminance_target_ = std::move(target);
+  luminance_blend_start_ = last_step_time_.is_null() ? base::TimeTicks::Now()
+                                                     : last_step_time_;
+  luminance_blending_ = true;
+}
+
+void WindriseEdgeLightView::ApplyFallbackLuminance() {
+  // 取不到网页图：theme-color -> 网页背景色 -> 0.95。
+  float luma = 0.95f;
+  if (web_contents_) {
+    std::optional<SkColor> color = web_contents_->GetThemeColor();
+    if (!color) {
+      color = web_contents_->GetBackgroundColor();
+    }
+    if (color) {
+      luma = (0.2126f * SkColorGetR(*color) + 0.7152f * SkColorGetG(*color) +
+              0.0722f * SkColorGetB(*color)) /
+             255.0f;
+    }
+  }
+  luminance_ = EdgeLuminance();
+  luminance_.valid = true;
+  luminance_.overall = luma;
+  luminance_.point_luminance.assign(browseros::windrise::kEdgeLightPoints, luma);
+  luminance_blending_ = false;
+}
+
+void WindriseEdgeLightView::BlendLuminance(base::TimeTicks timestamp) {
+  if (!luminance_blending_) {
+    return;
+  }
+  const float f = static_cast<float>(
+      std::clamp((timestamp - luminance_blend_start_).InSecondsF() /
+                     kLuminanceBlendSeconds,
+                 0.0, 1.0));
+  const auto& to = luminance_target_.point_luminance;
+  if (luminance_from_.size() == to.size() &&
+      luminance_.point_luminance.size() == to.size()) {
+    for (size_t i = 0; i < to.size(); ++i) {
+      luminance_.point_luminance[i] = Lerp(luminance_from_[i], to[i], f);
+    }
+  }
+  luminance_.overall = Lerp(overall_from_, luminance_target_.overall, f);
+  if (f >= 1.0f) {
+    luminance_blending_ = false;
+  }
+}
+
+void WindriseEdgeLightView::UpdateDegradeLevel(base::TimeDelta paint_cost) {
+  // 预算（方案）：CPU <= 0.5ms。超出先降到 60 帧，再超降到 30 帧并去掉第 4、8 层
+  // 与微粒。只升不降（GPU 状态变化除外，那条走 gpu_blur_ok_）。
+  paint_cost_average_ = paint_cost_average_ * 9 / 10 + paint_cost / 10;
+  if (degrade_level_ < 2 && paint_cost_average_ > base::Milliseconds(3)) {
+    degrade_level_ = 2;
+  } else if (degrade_level_ < 1 && paint_cost_average_ > base::Milliseconds(1)) {
+    degrade_level_ = 1;
+  }
+}
+
+// ---------------------------------------------------------------------------
+// 绘制
+// ---------------------------------------------------------------------------
+void WindriseEdgeLightView::OnPaint(gfx::Canvas* canvas) {
+  if (phase_ == Phase::kHidden || appear_ <= 0.0f) {
+    return;
+  }
+  base::ElapsedTimer timer;
+  FrameState frame;
+  frame.time = effect_time_;
+  frame.appear = appear_;
+  frame.pulse_age = reduced_motion_ ? -1.0f : pulse_age_;
+  frame.ripple_source_s = ripple_source_s_;
+  frame.breath_phase = effect_time_;
+  frame.reduced_motion = reduced_motion_;
+  frame.blur_allowed = gpu_blur_ok_;
+  frame.degrade_level = std::max(degrade_level_, gpu_blur_ok_ ? 0 : 2);
+  PaintFrame(canvas, frame);
+  UpdateDegradeLevel(timer.Elapsed());
+}
+
+void WindriseEdgeLightView::PaintFrame(gfx::Canvas* gfx_canvas,
+                                       const FrameState& frame) {
+  UpdatePerimeter();
+  if (perimeter_.empty() || frame.appear <= 0.0f) {
+    return;
+  }
+  const EdgeLightTier& tier = browseros::windrise::ActiveTier();
+  using browseros::windrise::GlintLuminance;
+  using browseros::windrise::PulseEnvelope;
+  using browseros::windrise::RippleLuminance;
+  using browseros::windrise::SilverWave;
+  using browseros::windrise::SilverWaveDepth;
+  using browseros::windrise::Wave;
+
+  Ctx c;
+  c.canvas = gfx_canvas->sk_canvas();
+  c.perimeter = &perimeter_;
+  c.luminance = &luminance_;
+  c.tier = &tier;
+  c.t = frame.time;
+  c.k = Clamp01(frame.appear);
+  c.env = frame.reduced_motion ? 0.0f : PulseEnvelope(frame.pulse_age);
+  c.wide = 1.0f + (tier.pulse_w - 1.0f) * c.env;
+  c.breath = frame.reduced_motion
+                 ? 1.0f
+                 : Clamp01(1.0f - tier.breath +
+                           tier.breath * (0.5f + 0.5f * std::sin(kTwoPi * frame.breath_phase / 3.4f)) +
+                           c.env);
+  c.edge_fade = SmoothStep(0.0f, 0.15f, c.k);
+  c.lt = luminance_.OverallLightness();
+  c.blur = frame.blur_allowed;
+  c.level = frame.degrade_level;
+
+  const int n = N(c);
+  const auto& pts = perimeter_.points();
+  const float k = c.k;
+  const float t = c.t;
+
+  // 每个点共用的两条曲线先算好。
+  std::vector<float> wv(n), sw(n), dep(n);
+  for (int i = 0; i < n; ++i) {
+    wv[i] = Wave(pts[i].s, t, 1.0f);
+    sw[i] = SilverWave(pts[i].s, t);
+    dep[i] = SilverWaveDepth(pts[i].s, t) * k * (1.0f + tier.surge * c.env);
+  }
+  auto depths = [&](auto fn) {
+    std::vector<float> d(n);
+    for (int i = 0; i < n; ++i) {
+      d[i] = fn(i);
+    }
+    return d;
+  };
+
+  // 外沿裁成网页角的圆角矩形（外沿那圈贴着网页角）。
+  const float w = static_cast<float>(width());
+  const float h = static_cast<float>(height());
+  SkVector radii[4] = {{perimeter_radii_[0], perimeter_radii_[0]},
+                       {perimeter_radii_[1], perimeter_radii_[1]},
+                       {perimeter_radii_[2], perimeter_radii_[2]},
+                       {perimeter_radii_[3], perimeter_radii_[3]}};
+  SkRRect clip;
+  clip.setRectRadii(SkRect::MakeWH(w, h), radii);
+  c.canvas->save();
+  c.canvas->clipRRect(clip, /*do_anti_alias=*/true);
+
+  // 全部光层画在一个离屏组里，最后统一抠文字（降档后不抠）。
+  const bool use_text_mask = c.level < 2 && !luminance_.text_rects.empty();
+  if (use_text_mask) {
+    c.canvas->saveLayer(SkRect::MakeWH(w, h), cc::PaintFlags());
+  }
+
+  const float fade = c.edge_fade;
+  const float lt = c.lt;
+
+  // 1 托底冷灰柔影（只在浅色页，按段定浓淡）
+  if (c.blur && c.level < 2 && lt > 0.02f) {
+    std::vector<float> d1 = depths([&](int i) {
+      return (1.5f + wv[i] * 8.4f) * k * (1.0f + c.env) * 1.35f + 3.0f;
+    });
+    const std::vector<SkPoint> front = LoopAtDepth(c, d1);
+    const int segments = (n + browseros::windrise::kEdgeLightSegmentPoints - 1) /
+                         browseros::windrise::kEdgeLightSegmentPoints;
+    cc::PaintFlags group;
+    SetBlur(c, &group, 10.0f);
+    c.canvas->saveLayer(group);
+    for (int seg = 0; seg < segments; ++seg) {
+      const float seg_lt = luminance_.LightnessOfSegment(seg);
+      if (seg_lt < 0.02f) {
+        continue;
+      }
+      const int begin = seg * browseros::windrise::kEdgeLightSegmentPoints;
+      const int end = begin + browseros::windrise::kEdgeLightSegmentPoints;
+      cc::PaintFlags f;
+      f.setAntiAlias(true);
+      f.setColor(Rgba(132, 143, 158, 0.74f * seg_lt * fade));
+      c.canvas->drawPath(BandPath(c, front, begin, end), f);
+    }
+    c.canvas->restore();
+  }
+
+  // 2 官网白光 soft（两条带子，相位 0 / 0.37，方向 +1 / -1）
+  if (c.blur) {
+    FillBand(c, depths([&](int i) { return (1.5f + Wave(pts[i].s, t, 1.0f) * 8.4f) * k * c.wide; }),
+             Rgba(255, 255, 255, 0.60f * c.breath * fade), 9.0f);
+    FillBand(c, depths([&](int i) { return (0.9f + Wave(pts[i].s + 0.37f, t, -1.0f) * 4.8f) * k * c.wide; }),
+             Rgba(255, 255, 255, 0.45f * c.breath * fade), 9.0f);
+    // 3 官网白光 mid
+    FillBand(c, depths([&](int i) { return (0.6f + Wave(pts[i].s + 0.11f, t, 1.0f) * 3.6f) * k * c.wide; }),
+             Rgba(255, 255, 255, 0.70f * c.breath * fade), 3.0f);
+  }
+
+  // 脉冲 / 高光 / 涟漪的逐点亮度
+  std::vector<float> glint(n, 0.0f), ripple(n, 0.0f);
+  bool any_glint = false, any_ripple = false;
+  if (c.level < 2) {  // 高光只用于第 4、8 层，降档后这两层不画
+    for (int i = 0; i < n; ++i) {
+      glint[i] = GlintLuminance(tier, pts[i].s, t) * tier.glint_a;
+      any_glint = any_glint || glint[i] > 0.02f;
+    }
+  }
+  if (frame.ripple_source_s && c.env > 0.0f && frame.pulse_age >= 0.0f) {
+    for (int i = 0; i < n; ++i) {
+      ripple[i] = RippleLuminance(tier, pts[i].s, *frame.ripple_source_s, frame.pulse_age) * tier.ripple_a;
+      any_ripple = any_ripple || ripple[i] > 0.02f;
+    }
+  }
+
+  // 4 脉冲闪光 + 浪头高光的晕 + 涟漪的晕（降档后去掉）
+  if (c.blur && c.level < 2) {
+    const SkColor glow_color = LerpRgb(214, 222, 234, 255, 255, 255, lt, 1.0f);
+    if (c.env > 0.01f) {
+      FillBand(c, depths([&](int i) { return (4.0f + wv[i] * 12.0f) * c.wide * k; }),
+               Rgba(255, 255, 255, tier.flash_a * c.env * fade), 6.0f);
+    }
+    if (any_glint) {
+      std::vector<float> g(n);
+      for (int i = 0; i < n; ++i) {
+        g[i] = glint[i] * tier.glow_a;
+      }
+      DrawContinuousLine(c, depths([&](int i) { return dep[i] - 1.1f; }), 7.0f,
+                         Rgba(SkColorGetR(glow_color), SkColorGetG(glow_color), SkColorGetB(glow_color), fade),
+                         g, 5.0f, 3.0f);
+    }
+    if (any_ripple) {
+      DrawContinuousLine(c, depths([&](int i) { return dep[i] - 1.1f; }), 10.0f,
+                         Rgba(255, 255, 255, fade), ripple, 5.0f, 5.0f);
+    }
+  }
+
+  // 5 银浪浪身（浅色页偏冷灰，深色页偏亮银；按整圈浅色程度过渡）
+  FillBand(c, dep, LerpRgb(178, 190, 206, 150, 161, 176, lt, Lerp(0.34f, 0.46f, lt) * fade), 1.2f);
+  FillBand(c, depths([&](int i) { return (0.8f + SilverWave(pts[i].s + 0.013f, t) * 8.0f) * k; }),
+           LerpRgb(200, 210, 224, 124, 136, 153, lt, Lerp(0.28f, 0.34f, lt) * fade), 1.2f);
+  DrawContinuousLine(c, depths([&](int i) { return (1.0f + SilverWave(pts[i].s + 0.31f, t * 1.15f) * 10.0f) * k; }),
+                     1.2f, Rgba(255, 255, 255, 0.45f * fade), {}, 0.0f, 0.0f);
+
+  // 6 浪头弯月边：沿 dep - 1.1 一条连续白线；浅色页在 dep + 0.9 再一道冷灰影线
+  std::vector<float> crest(n);
+  for (int i = 0; i < n; ++i) {
+    crest[i] = 0.25f + 0.75f * Clamp01(sw[i]);
+  }
+  DrawContinuousLine(c, depths([&](int i) { return dep[i] - 1.1f; }), 1.0f,
+                     Rgba(255, 255, 255, 0.78f * fade), crest, 2.0f, 0.4f);
+  if (lt > 0.05f) {
+    DrawContinuousLine(c, depths([&](int i) { return dep[i] + 0.9f; }), 1.0f,
+                       Rgba(108, 120, 137, 0.42f * lt * fade), crest, 2.0f, 0.4f);
+  }
+
+  // 7 官网白光 fine：两条细丝，方向相反
+  std::vector<float> fine_a(n), fine_b(n);
+  for (int i = 0; i < n; ++i) {
+    fine_a[i] = 0.45f + 0.55f * Clamp01(wv[i] / 1.2f);
+    const float wb = Wave(pts[i].s, t, -1.0f);
+    fine_b[i] = 0.45f + 0.55f * Clamp01(wb / 1.2f);
+  }
+  DrawContinuousLine(c, depths([&](int i) { return (0.9f + wv[i] * 4.5f) * k * c.wide; }), 1.9f,
+                     Rgba(255, 255, 255, 0.95f * fade), fine_a, 3.0f, 0.5f);
+  DrawContinuousLine(c, depths([&](int i) { return (0.6f + Wave(pts[i].s, t, -1.0f) * 2.7f) * k * c.wide; }),
+                     1.2f, Rgba(255, 255, 255, 0.70f * fade), fine_b, 3.0f, 0.5f);
+
+  // 8 浪头高光 + 涟漪芯（降档后去掉）
+  if (c.level < 2) {
+    if (any_glint) {
+      DrawContinuousLine(c, depths([&](int i) { return dep[i] - 1.0f; }), 1.6f,
+                         Rgba(255, 255, 255, fade), glint, 2.0f, 0.35f);
+    }
+    if (any_ripple) {
+      DrawContinuousLine(c, depths([&](int i) { return dep[i] - 1.0f; }), 1.8f,
+                         Rgba(255, 255, 255, fade), ripple, 2.0f, 0.35f);
+    }
+  }
+
+  // 9 光粒 + 微粒：浅色处纯白不透明小点（只在银浪带里出现，淡入淡出靠变小，
+  // 半径 < 0.55px 不画）；深色处亮银。
+  {
+    std::vector<EdgeParticle> particles;
+    browseros::windrise::ComputeSpray(tier, t, tier.burst * c.env, &particles);
+    if (c.level < 2) {
+      std::vector<EdgeParticle> motes;
+      browseros::windrise::ComputeMotes(tier, t, &motes);
+      particles.insert(particles.end(), motes.begin(), motes.end());
+    }
+    const bool light_page = lt >= 0.5f;
+    cc::PaintFlags dot;
+    dot.setAntiAlias(true);
+    dot.setStyle(cc::PaintFlags::kFill_Style);
+    if (!light_page) {
+      SetBlur(c, &dot, 0.45f);
+    }
+    for (size_t i = 0; i < particles.size(); ++i) {
+      EdgeParticle p = particles[i];
+      if (p.alpha <= 0.01f) {
+        continue;
+      }
+      // 退场「由密到疏」：每颗按自己的哈希先后消失。
+      if (k < 1.0f && browseros::windrise::Hash01(static_cast<uint32_t>(i), 99) > k * 1.3f) {
+        continue;
+      }
+      const EdgePoint base = perimeter_.At(p.s);
+      float radius = p.radius;
+      if (light_page) {
+        const float front = SilverWaveDepth(p.s, t) * k * (1.0f + tier.surge * c.env);
+        radius *= Clamp01((front - p.depth) / 3.0f);
+        if (radius < 0.55f) {
+          continue;
+        }
+        dot.setColor(Rgba(255, 255, 255, fade));
+      } else {
+        dot.setColor(Rgba(225, 232, 244, Clamp01(p.alpha) * fade));
+      }
+      const float d = std::min(p.depth * k, base.depth_cap);
+      DrawDot(c.canvas, dot, base.x + base.nx * d, base.y + base.ny * d, radius);
+    }
+  }
+
+  if (use_text_mask) {
+    // 压字：网页图里有字的地方抠淡 82%（外沿 4px 以内的格子已经被排除在外）。
+    cc::PaintFlags mask;
+    mask.setBlendMode(SkBlendMode::kDstOut);
+    mask.setColor(Rgba(0, 0, 0, 0.82f));
+    for (const SkRect& r : luminance_.text_rects) {
+      c.canvas->drawRect(r, mask);
+    }
+    c.canvas->restore();
+  }
+  c.canvas->restore();
+}
+
+BEGIN_METADATA(WindriseEdgeLightView)
+END_METADATA
