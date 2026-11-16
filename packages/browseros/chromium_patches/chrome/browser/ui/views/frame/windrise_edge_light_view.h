diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_view.h b/chrome/browser/ui/views/frame/windrise_edge_light_view.h
new file mode 100644
index 0000000000..ba9ba75e0c
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_view.h
@@ -0,0 +1,103 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_VIEW_H_
+#define CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_VIEW_H_
+
+#include "base/functional/callback.h"
+#include "base/memory/raw_ptr.h"
+#include "base/scoped_observation.h"
+#include "base/time/time.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/compositor/compositor.h"
+#include "ui/compositor/compositor_animation_observer.h"
+#include "ui/gfx/geometry/rounded_corners_f.h"
+#include "ui/views/view.h"
+
+namespace content {
+class WebContents;
+}  // namespace content
+
+// 风桥 AI 操作某个标签时的「计时器」。本身不画任何东西。
+//
+// 光效改由网页代码画（11-13 定）：默认是扩展画在网页里（页内层），也可以由
+// WindriseEdgeLightWebView（独立层）画；原生 Skia 画法已摘除。这个类只留下计时：
+//   - 开（SetShowing(true)）：300ms 起，cubic-bezier(.23, 1, .32, 1)；
+//   - 收：400ms，网页角按同一条曲线收回（照原实现，用户点过头的过渡，数值不改）；
+//   - 网页角 12 -> 36 的进度经 SetCornerProgressCallback() 每帧交给 ContentsContainerView；
+//   - IsActive()（含 400ms 收的过程）给独立层决定显示 / 隐藏；
+//   - 系统「减少动态效果」：直接到 36、不过渡；收时直接回去。
+// 时间按合成器每个 vsync 的时间戳算（ui::CompositorAnimationObserver）。
+// 本视图一直不可见、不接输入，布局只是给它一个位置。
+class WindriseEdgeLightView : public views::View,
+                              public ui::CompositorAnimationObserver {
+  METADATA_HEADER(WindriseEdgeLightView, views::View)
+
+ public:
+  using CornerProgressCallback = base::RepeatingCallback<void(float)>;
+
+  WindriseEdgeLightView();
+  WindriseEdgeLightView(const WindriseEdgeLightView&) = delete;
+  WindriseEdgeLightView& operator=(const WindriseEdgeLightView&) = delete;
+  ~WindriseEdgeLightView() override;
+
+  // 开 / 关。关 = 开始 400ms 收（收完才算不活跃）。重复调用幂等。
+  void SetShowing(bool showing);
+  // 目标状态：开着或正在起时为真。
+  bool IsShowing() const { return wants_showing_; }
+  // 是否活跃（含收的过程）。独立层跟它显示 / 隐藏。
+  bool IsActive() const { return phase_ != Phase::kHidden; }
+
+  // 网页区当前四角半径（只记下，不画）。
+  void SetCornerRadii(const gfx::RoundedCornersF& radii);
+
+  // 网页角过渡进度 0..1（0 = 基准，1 = AI 操作时的 36）。每帧变化时回调；收完回到
+  // 0 时一定回调一次。
+  void SetCornerProgressCallback(CornerProgressCallback callback);
+  float corner_progress() const { return corner_progress_; }
+
+  // 保留接口（原实现取亮度缩略图 / 画脉冲用），现在什么都不做：光效和脉冲由网页代码画。
+  void SetWebContents(content::WebContents* web_contents);
+  void Pulse(const browseros::windrise::AiPulse& pulse);
+
+  // views::View:
+  void AddedToWidget() override;
+  void RemovedFromWidget() override;
+
+  // ui::CompositorAnimationObserver:
+  void OnAnimationStep(base::TimeTicks timestamp) override;
+  void OnCompositingShuttingDown(ui::Compositor* compositor) override;
+
+ private:
+  enum class Phase { kHidden, kRampingUp, kSteady, kRampingDown };
+
+  void Start();
+  void BeginRampDown();
+  void Hide();
+  void ObserveCompositor();
+  void StopObservingCompositor();
+  // 开 / 关请求在没有 widget（没有合成器）时先记下，进了 widget 再生效。
+  void ApplyWantedState();
+  void NotifyCornerProgress(bool force);
+
+  bool wants_showing_ = false;
+  bool reduced_motion_ = false;
+  Phase phase_ = Phase::kHidden;
+
+  // 起 / 收的计时：phase_start_ 在下一次 OnAnimationStep 里取时间戳。
+  bool phase_start_pending_ = false;
+  base::TimeTicks phase_start_;
+  float corner_progress_ = 0.0f;
+  float corner_from_ = 0.0f;
+  float last_notified_corner_ = -1.0f;
+  CornerProgressCallback corner_callback_;
+
+  gfx::RoundedCornersF corner_radii_;
+
+  base::ScopedObservation<ui::Compositor, ui::CompositorAnimationObserver>
+      compositor_observation_{this};
+  raw_ptr<ui::Compositor> compositor_ = nullptr;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_VIEW_H_
