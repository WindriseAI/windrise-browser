diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_view.h b/chrome/browser/ui/views/frame/windrise_edge_light_view.h
new file mode 100644
index 0000000000..09706d70b6
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_view.h
@@ -0,0 +1,182 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_VIEW_H_
+#define CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_VIEW_H_
+
+#include <array>
+#include <optional>
+#include <utility>
+#include <vector>
+
+#include "base/functional/callback.h"
+#include "base/memory/raw_ptr.h"
+#include "base/memory/weak_ptr.h"
+#include "base/scoped_observation.h"
+#include "base/time/time.h"
+#include "base/timer/timer.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/ui/views/frame/windrise_edge_light_model.h"
+#include "content/public/browser/gpu_data_manager_observer.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/compositor/compositor.h"
+#include "ui/compositor/compositor_animation_observer.h"
+#include "third_party/skia/include/core/SkBitmap.h"
+#include "ui/gfx/geometry/rounded_corners_f.h"
+#include "ui/gfx/geometry/size_f.h"
+#include "ui/views/view.h"
+
+namespace content {
+class GpuDataManager;
+class WebContents;
+}  // namespace content
+
+// 风桥 N11：AI 操作某个标签时，网页区四周的边缘光效（官网流光 + 银色浪 + 粒子）。
+//
+// 依据：browser-design/native/方案.md「光效层（N11）」。不继承 glic 的
+// AnimatedEffectView（它是着色器路线，新方案是 Skia 路径 + 模糊），自己实现
+// 一个 views::View + ui::CompositorAnimationObserver：每个 vsync 一次
+// OnAnimationStep，时间用时间戳差。
+//
+// 本层 = 网页区外扩 0（光只画在网页区里），不接输入。外沿贴着网页角：四角半径由
+// ContentsContainerView 通过 SetCornerRadii() 给，走线、层的裁切、网页内容的圆角
+// 用的是同一组数（N10：AI 操作时网页角 12 -> 36 的同一个动画值）。
+//
+// 动画值（起 300ms 强缓出、收 400ms 线性、网页角过渡）都在本类里按合成器时间戳
+// 算，网页角过渡的进度通过 SetCornerProgressCallback() 的回调交给容器。
+class WindriseEdgeLightView : public views::View,
+                              public ui::CompositorAnimationObserver,
+                              public content::GpuDataManagerObserver {
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
+  // 开 / 关。关 = 开始 400ms 淡出（淡出结束才真的隐藏）。重复调用幂等。
+  void SetShowing(bool showing);
+  // 目标状态（不是「现在看得见」）：开着或正在淡入时为真。
+  bool IsShowing() const { return wants_showing_; }
+  // 层是否可见（含淡出中）。布局用它决定可见性，不能用 IsShowing()。
+  bool IsActive() const { return phase_ != Phase::kHidden; }
+
+  // 网页区当前四角半径（逻辑像素，已含 N10 的 AI 过渡）。
+  void SetCornerRadii(const gfx::RoundedCornersF& radii);
+
+  // 网页角过渡进度 0..1（0 = 基准，1 = AI 操作时的 36）。每帧变化时回调；淡出
+  // 结束回到 0 时一定回调一次。
+  void SetCornerProgressCallback(CornerProgressCallback callback);
+  float corner_progress() const { return corner_progress_; }
+
+  // 取亮度缩略图的标签（只存弱指针，不拥有）。
+  void SetWebContents(content::WebContents* web_contents);
+
+  // AI 的一次动作。没在显示（含淡出中）、减少动态效果时忽略。
+  void Pulse(const browseros::windrise::AiPulse& pulse);
+
+  // views::View:
+  void AddedToWidget() override;
+  void RemovedFromWidget() override;
+  void OnPaint(gfx::Canvas* canvas) override;
+
+  // ui::CompositorAnimationObserver:
+  void OnAnimationStep(base::TimeTicks timestamp) override;
+  void OnCompositingShuttingDown(ui::Compositor* compositor) override;
+
+  // content::GpuDataManagerObserver:
+  void OnGpuInfoUpdate() override;
+
+  // 给测试 / 静态一帧用：直接按给定时刻与进度画一帧，不依赖合成器。
+  struct FrameState {
+    float time = browseros::windrise::kEdgeLightFreezeTime;  // 光效时间（秒）
+    float appear = 1.0f;                                       // k，0..1
+    float pulse_age = -1.0f;                                   // 秒，<0 无脉冲
+    std::optional<float> ripple_source_s;                      // 涟漪起点
+    float breath_phase = 0.0f;                                 // 秒
+    bool reduced_motion = true;                                // 不呼吸 / 不脉冲
+    bool blur_allowed = true;                                  // GPU 可用
+    int degrade_level = 0;                                     // 0 全 / 1 / 2
+  };
+  void PaintFrame(gfx::Canvas* canvas, const FrameState& frame);
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
+
+  void UpdatePerimeter();
+  void NotifyCornerProgress(bool force);
+
+  void RequestThumbnail();
+  void OnThumbnail(const SkBitmap& bitmap);
+  void ApplyFallbackLuminance();
+  void BlendLuminance(base::TimeTicks timestamp);
+
+  void UpdateDegradeLevel(base::TimeDelta paint_cost);
+
+  bool wants_showing_ = false;
+  Phase phase_ = Phase::kHidden;
+
+  // 起 / 收的计时：phase_start_ 在下一次 OnAnimationStep 里取时间戳。
+  bool phase_start_pending_ = false;
+  base::TimeTicks phase_start_;
+  float appear_ = 0.0f;       // k
+  float appear_from_ = 0.0f;  // 本段开始时的 k
+  float corner_progress_ = 0.0f;
+  float corner_from_ = 0.0f;
+  float last_notified_corner_ = -1.0f;
+  CornerProgressCallback corner_callback_;
+
+  // 光效时间：从第一次显示算起，淡出 / 再开不归零。
+  base::TimeTicks effect_start_;
+  float effect_time_ = 0.0f;
+  bool reduced_motion_ = false;
+
+  // 脉冲。
+  base::TimeTicks pulse_time_;
+  float pulse_age_ = -1.0f;
+  std::optional<float> ripple_source_s_;
+
+  gfx::RoundedCornersF corner_radii_;
+  browseros::windrise::EdgePerimeter perimeter_;
+  gfx::SizeF perimeter_size_;
+  std::array<float, 4> perimeter_radii_ = {0, 0, 0, 0};
+
+  // 亮度：当前值（画的用它）与目标值（刚取到的），300ms 线性插值。
+  browseros::windrise::EdgeLuminance luminance_;
+  browseros::windrise::EdgeLuminance luminance_target_;
+  std::vector<float> luminance_from_;
+  float overall_from_ = 0.95f;
+  base::TimeTicks luminance_blend_start_;
+  bool luminance_blending_ = false;
+  base::WeakPtr<content::WebContents> web_contents_;
+  base::RepeatingTimer thumbnail_timer_;
+
+  // 预算与退路。
+  bool gpu_blur_ok_ = true;
+  int degrade_level_ = 0;
+  base::TimeTicks last_paint_time_;
+  base::TimeDelta paint_cost_average_;
+  base::TimeTicks last_step_time_;
+
+  base::ScopedObservation<ui::Compositor, ui::CompositorAnimationObserver>
+      compositor_observation_{this};
+  raw_ptr<ui::Compositor> compositor_ = nullptr;
+  base::ScopedObservation<content::GpuDataManager,
+                          content::GpuDataManagerObserver>
+      gpu_observation_{this};
+
+  base::WeakPtrFactory<WindriseEdgeLightView> weak_factory_{this};
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_VIEW_H_
