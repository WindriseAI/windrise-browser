diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_model.h b/chrome/browser/ui/views/frame/windrise_edge_light_model.h
new file mode 100644
index 0000000000..ff64c94369
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_model.h
@@ -0,0 +1,175 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_MODEL_H_
+#define CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_MODEL_H_
+
+#include <array>
+#include <cstdint>
+#include <vector>
+
+#include "third_party/skia/include/core/SkBitmap.h"
+#include "third_party/skia/include/core/SkRect.h"
+
+// 风桥 N11 边缘光效的纯计算部分：走线、起伏函数、脉冲包络、粒子、亮度分析。
+// 不依赖任何 View / 合成器，输入全是数，输出是数，同一时刻算出来永远一样
+// （方案「粒子怎么算」：全部由时间算出来，不存状态）。依据只有一份：
+// browser-design/native/方案.md「光效层（N11）」一节。
+namespace browseros::windrise {
+
+// ---------------------------------------------------------------------------
+// 强度档（方案「参数表」两列）。用户还没选克制 / 更炫：两套都编进来，由下面
+// 一个编译期常量切换，默认「克制」。不加偏好项。
+// ---------------------------------------------------------------------------
+enum class EdgeLightIntensity { kCalm = 1, kBold = 2 };
+inline constexpr EdgeLightIntensity kEdgeLightIntensity =
+    EdgeLightIntensity::kCalm;
+
+struct EdgeLightTier {
+  int spray;           // 光粒槽位
+  int motes;           // 微粒
+  int glints;          // 浪头高光段数
+  float glint_a;       // 高光亮度
+  float glow_a;        // 高光的晕
+  float pulse_w;       // 脉冲：向内柔光宽度倍数 W
+  float surge;         // 脉冲：银浪涌
+  float flash_a;       // 脉冲：闪光
+  float ripple_a;      // 涟漪亮度
+  float ripple_v;      // 涟漪速度（圈 / 秒）
+  float burst;         // 脉冲时光粒加量
+  float spray_a;       // 光粒亮度
+  float mote_a;        // 微粒亮度
+  float particle_r;    // 粒子半径倍数
+  float breath;        // 白光呼吸幅度
+};
+
+inline constexpr EdgeLightTier kCalmTier = {
+    160, 160, 4, 0.75f, 0.25f, 1.4f, 0.5f, 0.35f,
+    0.55f, 0.45f, 0.0f, 0.75f, 0.40f, 1.0f, 0.10f};
+inline constexpr EdgeLightTier kBoldTier = {
+    560, 480, 10, 1.00f, 0.65f, 1.8f, 1.0f, 0.70f,
+    1.00f, 0.55f, 0.8f, 1.00f, 0.60f, 1.3f, 0.20f};
+
+inline constexpr const EdgeLightTier& ActiveTier() {
+  return kEdgeLightIntensity == EdgeLightIntensity::kBold ? kBoldTier
+                                                          : kCalmTier;
+}
+
+// 共用常数（方案「共用」一行与各节）。
+inline constexpr int kEdgeLightPoints = 560;       // 周长取点数
+inline constexpr float kEdgeLightFreezeTime = 2.35f;  // 减少动态效果：定格时刻
+inline constexpr float kEdgeLightOuterBand = 30.0f;   // 带子向边外多少
+inline constexpr int kEdgeLightSegmentPoints = 14;    // 托底柔影分段点数
+inline constexpr float kEdgeLightMaxDepth = 26.0f;    // 光粒最深约 26px
+
+// ---------------------------------------------------------------------------
+// 走线：一圈圆角矩形（四角半径可以各不相同，0 = 直角），沿周长等距取
+// kEdgeLightPoints 个点；每点有位置、朝里的单位法线、占整圈比例 s 和「深度上限」
+// （圆角处向内偏移超过半径会自交成尖，所以深度不许超过该处半径 - 2）。
+// ---------------------------------------------------------------------------
+struct EdgePoint {
+  float x = 0;
+  float y = 0;
+  float nx = 0;  // 朝里的单位法线
+  float ny = 0;
+  float s = 0;
+  float depth_cap = 1000.0f;
+};
+
+class EdgePerimeter {
+ public:
+  // `radii` 顺序：左上、右上、右下、左下（同 gfx::RoundedCornersF 的构造顺序）。
+  // 半径会被夹到不超过宽高的一半。
+  void Build(float width, float height, const std::array<float, 4>& radii);
+
+  bool empty() const { return points_.empty(); }
+  const std::vector<EdgePoint>& points() const { return points_; }
+  float width() const { return width_; }
+  float height() const { return height_; }
+  float length() const { return length_; }
+
+  // 沿周长 s ∈ [0, 1) 的位置（线性插值，s 自动取小数部分）。
+  EdgePoint At(float s) const;
+
+  // 离 (x, y) 最近的那个走线点的 s。点是网页里的坐标，相对网页区左上角。
+  float NearestS(float x, float y) const;
+
+ private:
+  std::vector<EdgePoint> points_;
+  float width_ = 0;
+  float height_ = 0;
+  float length_ = 0;
+};
+
+// ---------------------------------------------------------------------------
+// 起伏函数、包络（方案「起伏函数」「脉冲」两节原样）。
+// ---------------------------------------------------------------------------
+float Wave(float s, float t, float dir);  // 官网原样
+float SilverWave(float s, float t);       // 银浪
+// 脉冲包络：age < 60ms 线性升到 1，之后 exp(-(age-60ms)/200ms)；900ms 截断。
+// age 为负或超时返回 0。
+float PulseEnvelope(float age_seconds);
+
+float SmoothStep(float e0, float e1, float x);
+float Clamp01(float x);
+// 0..1 的确定性哈希。
+float Hash01(uint32_t a, uint32_t b = 0, uint32_t c = 0);
+
+// ---------------------------------------------------------------------------
+// 粒子（由时间算出来，不存状态）。
+// ---------------------------------------------------------------------------
+struct EdgeParticle {
+  float s = 0;       // 沿周长位置
+  float depth = 0;   // 离边向里的距离（像素）
+  float radius = 0;  // 半径（像素）
+  float alpha = 0;
+};
+
+// 银浪浪身深度 dep(s, t) = (1.5 + SilverWave * 17)，不含 k 与脉冲涌。
+float SilverWaveDepth(float s, float t);
+
+// 光粒：从浪峰甩出。`burst_env` = 脉冲包络 * tier.burst（更炫档才非 0）。
+void ComputeSpray(const EdgeLightTier& tier,
+                  float t,
+                  float burst_env,
+                  std::vector<EdgeParticle>* out);
+// 微粒：沿白光漂。
+void ComputeMotes(const EdgeLightTier& tier,
+                  float t,
+                  std::vector<EdgeParticle>* out);
+
+// 浪头高光亮度（0..1），沿 s；多个窗心沿边滑、起落。
+float GlintLuminance(const EdgeLightTier& tier, float s, float t);
+// 涟漪亮度（0..1）：从 `source_s` 向两侧荡开一次、衰减、走不到半圈。
+float RippleLuminance(const EdgeLightTier& tier,
+                      float s,
+                      float source_s,
+                      float age_seconds);
+
+// ---------------------------------------------------------------------------
+// 亮度分析：从网页的一张缩略图（只在内存里）算出
+//  - 贴边一圈每个走线点的亮度（离边 6px 处，±6 点平滑）；
+//  - 文字遮罩：局部亮度跳变 > 0.16 的格子，外扩 8px，离边 4px 以外、70px 以内，
+//    按行合并成矩形。
+// 缩略图的宽高与网页区成比例即可（建议 1/4 大小，一个像素 ≈ 一格 4px）。
+// ---------------------------------------------------------------------------
+struct EdgeLuminance {
+  bool valid = false;
+  // 每个走线点的贴边亮度 0..1；valid 时长度 == kEdgeLightPoints。
+  std::vector<float> point_luminance;
+  // 整圈平均亮度。
+  float overall = 0.95f;
+  // 文字遮罩矩形（网页区坐标，逻辑像素）。
+  std::vector<SkRect> text_rects;
+
+  // 托底柔影的浓淡 lt = smoothstep(0.35, 0.8, 亮度)；没有数据时按亮 0.95。
+  float LightnessOfSegment(int segment) const;
+  float OverallLightness() const;
+};
+
+EdgeLuminance AnalyzeThumbnail(const SkBitmap& thumbnail,
+                               const EdgePerimeter& perimeter);
+
+}  // namespace browseros::windrise
+
+#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_WINDRISE_EDGE_LIGHT_MODEL_H_
