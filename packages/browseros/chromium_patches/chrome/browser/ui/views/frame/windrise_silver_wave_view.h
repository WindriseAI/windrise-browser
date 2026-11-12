diff --git a/chrome/browser/ui/views/frame/windrise_silver_wave_view.h b/chrome/browser/ui/views/frame/windrise_silver_wave_view.h
new file mode 100644
index 0000000..36bcb88
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_silver_wave_view.h
@@ -0,0 +1,67 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_SILVER_WAVE_VIEW_H_
+#define CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_SILVER_WAVE_VIEW_H_
+
+#include "base/time/time.h"
+#include "chrome/browser/glic/browser_ui/animated_effect_view.h"
+#include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/gfx/geometry/rounded_corners_f.h"
+
+class Profile;
+
+// 风桥 N11：AI 操作某个标签时，网页区四周的银色海浪光效。
+//
+// 复用 158 已编进的 glic::AnimatedEffectView（CompositorAnimationObserver，每个
+// vsync 一次），只换着色器资源 IDR_WINDRISE_SILVER_WAVE_SHADER；层的范围是网页区
+// 向外扩 `silver_wave_spread`（外溢辉光），不接输入。GPU 不可用 / 减少动态效果时
+// 退成一道静止银边（方案 N11）。
+//
+// 光效的开关由扩展经 browserOS.setAiActivity(tabId, on) 推，状态在
+// AiWindowState 里；ContentsContainerView 负责调用 SetShowing() 与给本层定位。
+class WindriseSilverWaveView : public glic::AnimatedEffectView {
+  METADATA_HEADER(WindriseSilverWaveView, glic::AnimatedEffectView)
+
+ public:
+  explicit WindriseSilverWaveView(Profile* profile);
+  WindriseSilverWaveView(const WindriseSilverWaveView&) = delete;
+  WindriseSilverWaveView& operator=(const WindriseSilverWaveView&) = delete;
+  ~WindriseSilverWaveView() override;
+
+  // 打开 / 关闭。打开时从 0 亮起（420ms），关闭时 420ms 淡出。
+  void SetShowing(bool showing);
+
+  // 网页区四角半径（逻辑像素）。上两角是网页区圆角，下两角跟窗口。
+  void SetCornerRadii(const gfx::RoundedCornersF& radii);
+  gfx::RoundedCornersF corner_radii() const { return corner_radii_; }
+
+  // 本层比网页区向外扩多少（逻辑像素）。布局时用，光效据此算距离场。
+  static int GetSpread();
+
+  // views::View:
+  void AddedToWidget() override;
+  void OnAnimationStep(base::TimeTicks timestamp) override;
+
+ protected:
+  // glic::AnimatedEffectView:
+  bool IsCycleDone(base::TimeTicks timestamp) const override;
+  base::TimeDelta GetTotalDuration() const override;
+  void PopulateShaderUniforms(
+      std::vector<cc::PaintShader::FloatUniform>& float_uniforms,
+      std::vector<cc::PaintShader::Float2Uniform>& float2_uniforms,
+      std::vector<cc::PaintShader::Float4Uniform>& float4_uniforms,
+      std::vector<cc::PaintShader::IntUniform>& int_uniforms) const override;
+  void DrawEffect(gfx::Canvas* canvas, const cc::PaintFlags& flags) override;
+  int GetShaderResourceId(bool simplified) const override;
+  base::TimeDelta GetRampUpDuration() const override;
+  base::TimeDelta GetRampDownDuration() const override;
+
+ private:
+  // 网页区在本层坐标系里的矩形（逻辑像素）：本层 = 网页区向外扩 Spread。
+  gfx::Rect ContentsRectInView() const;
+
+  gfx::RoundedCornersF corner_radii_;
+};
+
+#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_SILVER_WAVE_VIEW_H_
\ No newline at end of file
