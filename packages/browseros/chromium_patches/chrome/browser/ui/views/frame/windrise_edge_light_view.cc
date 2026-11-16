diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_view.cc b/chrome/browser/ui/views/frame/windrise_edge_light_view.cc
new file mode 100644
index 0000000000..863cbbdae7
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_view.cc
@@ -0,0 +1,213 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+//
+// 风桥 计时器：只计时、不绘制（11-13）。
+// 原生 Skia 光效已摘除（光效改由网页代码画：扩展页内层 / WindriseEdgeLightWebView 独立层），
+// 这里只保留原实现里的起 / 收计时和网页角 12 -> 36 的进度回调——曲线、时长照原实现，一个数都没改：
+//   起 300ms，cubic-bezier(.23, 1, .32, 1)；收 400ms，网页角按同一条曲线收回；减少动态效果时不过渡。
+// 和原实现唯一的写法差别：缓动曲线每次现建（原来用 base::NoDestructor 包了一个静态的
+// gfx::CubicBezier，在 158.0.8127.41 上触发 NoDestructor 的静态断言），算出来的值相同。
+
+#include "chrome/browser/ui/views/frame/windrise_edge_light_view.h"
+
+#include <algorithm>
+#include <cmath>
+#include <utility>
+
+#include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/gfx/animation/animation.h"
+#include "ui/gfx/animation/tween.h"
+#include "ui/views/widget/widget.h"
+
+namespace {
+
+// 方案：起 300ms 强缓出，收 400ms；网页角过渡 cubic-bezier(.23, 1, .32, 1)。
+constexpr double kRampUpSeconds = 0.300;
+constexpr double kRampDownSeconds = 0.400;
+
+float Ease(float x) {
+  // 不再每帧新建 CubicBezier，改用 T0（gfx::Tween::WINDRISE_EASE_OUT，
+  // 内部是同一条 CubicBezier(0.23, 1, 0.32, 1) 的静态实例），算出来的值逐位相同。
+  return static_cast<float>(gfx::Tween::CalculateValue(
+      gfx::Tween::WINDRISE_EASE_OUT, std::clamp(x, 0.0f, 1.0f)));
+}
+
+float Lerp(float a, float b, float t) {
+  return a + (b - a) * t;
+}
+
+}  // namespace
+
+WindriseEdgeLightView::WindriseEdgeLightView() {
+  // 不画、不接输入；布局只给它一个位置。
+  SetVisible(false);
+  SetCanProcessEventsWithinSubtree(false);
+  // 计时可能跨很久（AI 一直在操作），不让合成器因为「活跃超过 60 秒」报失败。
+  set_check_active_duration(false);
+}
+
+WindriseEdgeLightView::~WindriseEdgeLightView() = default;
+
+void WindriseEdgeLightView::SetShowing(bool showing) {
+  wants_showing_ = showing;
+  ApplyWantedState();
+}
+
+void WindriseEdgeLightView::SetCornerRadii(const gfx::RoundedCornersF& radii) {
+  corner_radii_ = radii;
+}
+
+void WindriseEdgeLightView::SetCornerProgressCallback(
+    CornerProgressCallback callback) {
+  corner_callback_ = std::move(callback);
+}
+
+void WindriseEdgeLightView::SetWebContents(content::WebContents* web_contents) {}
+
+void WindriseEdgeLightView::Pulse(const browseros::windrise::AiPulse& pulse) {}
+
+void WindriseEdgeLightView::AddedToWidget() {
+  views::View::AddedToWidget();
+  // 要求开的时候本视图还没进 widget（没有合成器）：这里补一次。
+  ApplyWantedState();
+  // 过渡进行中被挪出 / 挪回 widget（RemovedFromWidget 摘掉了观察者）：接着走。
+  if (phase_ == Phase::kRampingUp || phase_ == Phase::kRampingDown) {
+    phase_start_pending_ = true;
+    corner_from_ = corner_progress_;
+    ObserveCompositor();
+  }
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
+  corner_from_ = corner_progress_;
+  phase_start_pending_ = true;
+  if (reduced_motion_) {
+    // 减少动态效果：网页角直接到 36，不过渡。
+    phase_ = Phase::kSteady;
+    corner_progress_ = 1.0f;
+    NotifyCornerProgress(/*force=*/true);
+    StopObservingCompositor();
+  } else {
+    phase_ = Phase::kRampingUp;
+    ObserveCompositor();
+  }
+}
+
+void WindriseEdgeLightView::BeginRampDown() {
+  if (reduced_motion_) {
+    Hide();
+    return;
+  }
+  phase_ = Phase::kRampingDown;
+  corner_from_ = corner_progress_;
+  phase_start_pending_ = true;
+  ObserveCompositor();
+}
+
+void WindriseEdgeLightView::Hide() {
+  phase_ = Phase::kHidden;
+  corner_progress_ = 0.0f;
+  StopObservingCompositor();
+  // 回到基准时一定通知一次，网页角才会真的收回去（独立层也在这一刻隐藏）。
+  NotifyCornerProgress(/*force=*/true);
+}
+
+void WindriseEdgeLightView::ObserveCompositor() {
+  if (compositor_observation_.IsObserving()) {
+    return;
+  }
+  ui::Compositor* compositor =
+      GetWidget() ? GetWidget()->GetCompositor() : nullptr;
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
+void WindriseEdgeLightView::OnAnimationStep(base::TimeTicks timestamp) {
+  if (phase_start_pending_) {
+    phase_start_ = timestamp;
+    phase_start_pending_ = false;
+  }
+  const double elapsed = (timestamp - phase_start_).InSecondsF();
+  switch (phase_) {
+    case Phase::kRampingUp: {
+      const float x = static_cast<float>(
+          std::clamp(elapsed / kRampUpSeconds, 0.0, 1.0));
+      corner_progress_ = Lerp(corner_from_, 1.0f, Ease(x));
+      if (x >= 1.0f) {
+        // 到 36 以后不用每帧醒：停在这里，等收的时候再观察合成器。
+        phase_ = Phase::kSteady;
+        corner_progress_ = 1.0f;
+        NotifyCornerProgress(/*force=*/false);
+        StopObservingCompositor();
+        return;
+      }
+      break;
+    }
+    case Phase::kRampingDown: {
+      const float x = static_cast<float>(
+          std::clamp(elapsed / kRampDownSeconds, 0.0, 1.0));
+      corner_progress_ = corner_from_ * (1.0f - Ease(x));  // 网页角随光收回
+      if (x >= 1.0f) {
+        Hide();
+        return;
+      }
+      break;
+    }
+    case Phase::kSteady:
+      corner_progress_ = 1.0f;
+      StopObservingCompositor();
+      break;
+    case Phase::kHidden:
+      StopObservingCompositor();
+      return;
+  }
+  NotifyCornerProgress(/*force=*/false);
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
+BEGIN_METADATA(WindriseEdgeLightView)
+END_METADATA
