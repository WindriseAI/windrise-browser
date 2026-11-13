diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_model.cc b/chrome/browser/ui/views/frame/windrise_edge_light_model.cc
new file mode 100644
index 0000000000..9d2fc8015d
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_model.cc
@@ -0,0 +1,510 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/ui/views/frame/windrise_edge_light_model.h"
+
+#include <algorithm>
+#include <cmath>
+#include <utility>
+
+#include "third_party/skia/include/core/SkColor.h"
+
+namespace browseros::windrise {
+namespace {
+
+constexpr float kTwoPi = 6.28318530717958647692f;
+constexpr float kPi = 3.14159265358979323846f;
+
+uint32_t Mix(uint32_t h) {
+  h ^= h >> 16;
+  h *= 0x7feb352dU;
+  h ^= h >> 15;
+  h *= 0x846ca68bU;
+  h ^= h >> 16;
+  return h;
+}
+
+float Fract(float x) {
+  return x - std::floor(x);
+}
+
+// s 在圆周上的距离，0..0.5。
+float CircularDistance(float a, float b) {
+  float d = std::fabs(Fract(a) - Fract(b));
+  return d > 0.5f ? 1.0f - d : d;
+}
+
+float Luma(SkColor c) {
+  return (0.2126f * SkColorGetR(c) + 0.7152f * SkColorGetG(c) +
+          0.0722f * SkColorGetB(c)) /
+         255.0f;
+}
+
+struct Segment {
+  bool arc = false;
+  float length = 0;
+  // 直线：起点 + 方向 * d，法线固定。
+  float x0 = 0, y0 = 0, dx = 0, dy = 0, nx = 0, ny = 0;
+  // 圆弧：圆心、半径、起始角（弧度）。
+  float cx = 0, cy = 0, r = 0, a0 = 0;
+  float cap = 1000.0f;
+};
+
+}  // namespace
+
+float Clamp01(float x) {
+  return std::clamp(x, 0.0f, 1.0f);
+}
+
+float SmoothStep(float e0, float e1, float x) {
+  const float t = Clamp01((x - e0) / (e1 - e0));
+  return t * t * (3.0f - 2.0f * t);
+}
+
+float Hash01(uint32_t a, uint32_t b, uint32_t c) {
+  uint32_t h = Mix(a * 0x9E3779B1U + 0x85ebca6bU);
+  h = Mix(h ^ (b * 0xC2B2AE35U + 0x27d4eb2fU));
+  h = Mix(h ^ (c * 0x165667B1U + 0x9E3779B9U));
+  return static_cast<float>(h >> 8) * (1.0f / 16777216.0f);
+}
+
+// ---------------------------------------------------------------------------
+// 走线
+// ---------------------------------------------------------------------------
+void EdgePerimeter::Build(float width,
+                          float height,
+                          const std::array<float, 4>& radii_in) {
+  points_.clear();
+  width_ = std::max(0.0f, width);
+  height_ = std::max(0.0f, height);
+  length_ = 0;
+  if (width_ < 2 || height_ < 2) {
+    return;
+  }
+  const float max_r = std::min(width_, height_) * 0.5f;
+  const float rul = std::clamp(radii_in[0], 0.0f, max_r);
+  const float rur = std::clamp(radii_in[1], 0.0f, max_r);
+  const float rlr = std::clamp(radii_in[2], 0.0f, max_r);
+  const float rll = std::clamp(radii_in[3], 0.0f, max_r);
+  const float w = width_;
+  const float h = height_;
+
+  // 深度上限：圆角处向内偏移超过半径会自交；半径为 0（直角）不限。
+  auto cap_of = [](float r) { return r > 0 ? std::max(1.0f, r - 2.0f) : 1000.0f; };
+
+  std::vector<Segment> segs;
+  auto add_line = [&](float x0, float y0, float x1, float y1, float nx,
+                      float ny, float cap) {
+    Segment s;
+    s.length = std::hypot(x1 - x0, y1 - y0);
+    if (s.length <= 0.0001f) {
+      return;
+    }
+    s.x0 = x0;
+    s.y0 = y0;
+    s.dx = (x1 - x0) / s.length;
+    s.dy = (y1 - y0) / s.length;
+    s.nx = nx;
+    s.ny = ny;
+    s.cap = cap;
+    segs.push_back(s);
+  };
+  auto add_arc = [&](float cx, float cy, float r, float a0) {
+    if (r <= 0.0001f) {
+      return;
+    }
+    Segment s;
+    s.arc = true;
+    s.length = r * kPi * 0.5f;
+    s.cx = cx;
+    s.cy = cy;
+    s.r = r;
+    s.a0 = a0;
+    s.cap = cap_of(r);
+    segs.push_back(s);
+  };
+
+  // 顺时针，从上边左端开始。直线段的深度上限取它两端圆角里更小的那个。
+  add_line(rul, 0, w - rur, 0, 0, 1, std::min(cap_of(rul), cap_of(rur)));
+  add_arc(w - rur, rur, rur, -kPi * 0.5f);
+  add_line(w, rur, w, h - rlr, -1, 0, std::min(cap_of(rur), cap_of(rlr)));
+  add_arc(w - rlr, h - rlr, rlr, 0.0f);
+  add_line(w - rlr, h, rll, h, 0, -1, std::min(cap_of(rlr), cap_of(rll)));
+  add_arc(rll, h - rll, rll, kPi * 0.5f);
+  add_line(0, h - rll, 0, rul, 1, 0, std::min(cap_of(rll), cap_of(rul)));
+  add_arc(rul, rul, rul, kPi);
+
+  for (const Segment& s : segs) {
+    length_ += s.length;
+  }
+  if (length_ <= 0.0f) {
+    return;
+  }
+
+  points_.reserve(kEdgeLightPoints);
+  size_t seg_index = 0;
+  float seg_start = 0;
+  for (int i = 0; i < kEdgeLightPoints; ++i) {
+    const float d = length_ * (static_cast<float>(i) / kEdgeLightPoints);
+    while (seg_index + 1 < segs.size() &&
+           d >= seg_start + segs[seg_index].length) {
+      seg_start += segs[seg_index].length;
+      ++seg_index;
+    }
+    const Segment& s = segs[seg_index];
+    const float u = std::clamp(d - seg_start, 0.0f, s.length);
+    EdgePoint p;
+    p.s = static_cast<float>(i) / kEdgeLightPoints;
+    p.depth_cap = s.cap;
+    if (s.arc) {
+      const float a = s.a0 + (u / s.length) * kPi * 0.5f;
+      p.x = s.cx + s.r * std::cos(a);
+      p.y = s.cy + s.r * std::sin(a);
+      p.nx = -std::cos(a);
+      p.ny = -std::sin(a);
+    } else {
+      p.x = s.x0 + s.dx * u;
+      p.y = s.y0 + s.dy * u;
+      p.nx = s.nx;
+      p.ny = s.ny;
+    }
+    points_.push_back(p);
+  }
+}
+
+EdgePoint EdgePerimeter::At(float s) const {
+  if (points_.empty()) {
+    return EdgePoint();
+  }
+  const float f = Fract(s) * points_.size();
+  const size_t i0 = std::min(static_cast<size_t>(f), points_.size() - 1);
+  const size_t i1 = (i0 + 1) % points_.size();
+  const float t = f - static_cast<float>(i0);
+  const EdgePoint& a = points_[i0];
+  const EdgePoint& b = points_[i1];
+  EdgePoint p;
+  p.x = a.x + (b.x - a.x) * t;
+  p.y = a.y + (b.y - a.y) * t;
+  p.nx = a.nx + (b.nx - a.nx) * t;
+  p.ny = a.ny + (b.ny - a.ny) * t;
+  const float n = std::hypot(p.nx, p.ny);
+  if (n > 0.0001f) {
+    p.nx /= n;
+    p.ny /= n;
+  }
+  p.s = Fract(s);
+  p.depth_cap = std::min(a.depth_cap, b.depth_cap);
+  return p;
+}
+
+float EdgePerimeter::NearestS(float x, float y) const {
+  float best = 1e30f;
+  float best_s = 0;
+  for (const EdgePoint& p : points_) {
+    const float d = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
+    if (d < best) {
+      best = d;
+      best_s = p.s;
+    }
+  }
+  return best_s;
+}
+
+// ---------------------------------------------------------------------------
+// 起伏函数与包络
+// ---------------------------------------------------------------------------
+float Wave(float s, float t, float dir) {
+  const float a =
+      0.5f + 0.5f * std::sin(kTwoPi * (2.0f * s + dir * 0.09f * t) +
+                             1.4f * std::sin(kTwoPi * (3.0f * s - dir * 0.05f * t)));
+  const float b =
+      0.5f + 0.5f * std::sin(kTwoPi * (5.0f * s - dir * 0.13f * t) +
+                             0.8f * std::sin(kTwoPi * 2.0f * s + 0.3f * t));
+  return a * a * 0.75f + b * b * b * 0.45f;
+}
+
+float SilverWave(float s, float t) {
+  const float a =
+      0.5f + 0.5f * std::sin(kTwoPi * (3.0f * s + 0.06f * t) +
+                             1.2f * std::sin(kTwoPi * (5.0f * s - 0.045f * t)));
+  const float b =
+      0.5f + 0.5f * std::sin(kTwoPi * (7.0f * s - 0.10f * t) +
+                             0.9f * std::sin(kTwoPi * 2.0f * s + 0.23f * t));
+  const float c = 0.5f + 0.5f * std::sin(kTwoPi * (13.0f * s + 0.16f * t));
+  return a * a * 0.62f + b * b * 0.38f + c * 0.10f;
+}
+
+float PulseEnvelope(float age) {
+  if (age < 0.0f || age > 0.9f) {
+    return 0.0f;
+  }
+  if (age < 0.06f) {
+    return age / 0.06f;
+  }
+  return std::exp(-(age - 0.06f) / 0.2f);
+}
+
+float SilverWaveDepth(float s, float t) {
+  return 1.5f + SilverWave(s, t) * 17.0f;
+}
+
+// ---------------------------------------------------------------------------
+// 粒子
+// ---------------------------------------------------------------------------
+void ComputeSpray(const EdgeLightTier& tier,
+                  float t,
+                  float burst_env,
+                  std::vector<EdgeParticle>* out) {
+  out->clear();
+  out->reserve(tier.spray);
+  for (int slot = 0; slot < tier.spray; ++slot) {
+    const uint32_t u = static_cast<uint32_t>(slot);
+    const float life = 0.9f + 0.8f * Hash01(u, 1);
+    const float phase = Hash01(u, 2) * life;
+    const float dir = Hash01(u, 3) < 0.7f ? -1.0f : 1.0f;
+    const float speed = 0.005f + 0.006f * Hash01(u, 4);
+    const float out_dist = 5.0f + 10.0f * Hash01(u, 5);
+    const float radius = (0.8f + 0.8f * Hash01(u, 6)) * tier.particle_r;
+
+    const float shifted = t + phase;
+    const float age = std::fmod(shifted, life);
+    const uint32_t round = static_cast<uint32_t>(std::floor(shifted / life));
+    const float birth_time = std::max(0.0f, t - age);
+    const float birth_s = Hash01(u, round, 7);
+    const float h = SilverWave(birth_s, birth_time);
+    if (h < 0.55f) {
+      continue;
+    }
+    const float probability = (h - 0.55f) / 0.45f + 0.2f + burst_env;
+    if (Hash01(u, round, 8) >= probability) {
+      continue;
+    }
+    const float x = age / life;
+    EdgeParticle p;
+    p.s = Fract(birth_s + dir * speed * age);
+    p.depth = 0.9f * SilverWaveDepth(birth_s, birth_time) +
+              out_dist * (1.0f - (1.0f - x) * (1.0f - x)) * (1.0f + burst_env);
+    p.depth = std::min(p.depth, kEdgeLightMaxDepth);
+    p.radius = radius;
+    p.alpha = tier.spray_a * std::pow(std::sin(kPi * x), 1.3f) *
+              std::min(1.0f, 2.0f * (h - 0.5f));
+    out->push_back(p);
+  }
+}
+
+void ComputeMotes(const EdgeLightTier& tier,
+                  float t,
+                  std::vector<EdgeParticle>* out) {
+  out->clear();
+  out->reserve(tier.motes);
+  for (int slot = 0; slot < tier.motes; ++slot) {
+    const uint32_t u = static_cast<uint32_t>(slot) + 1000;
+    const float speed =
+        (0.0015f + 0.0025f * Hash01(u, 1)) * (Hash01(u, 2) < 0.5f ? -1.0f : 1.0f);
+    const float life = 3.0f + 3.0f * Hash01(u, 3);
+    const float phase = Hash01(u, 4) * life;
+    const float shifted = t + phase;
+    const float age = std::fmod(shifted, life);
+    const uint32_t round = static_cast<uint32_t>(std::floor(shifted / life));
+    const float s = Fract(Hash01(u, round, 5) + speed * age);
+    const float w = Wave(s, t, 1.0f);
+    EdgeParticle p;
+    p.s = s;
+    p.depth = (1.0f + w * 8.0f) + 0.5f;
+    p.radius = (0.5f + 0.5f * Hash01(u, 6)) * tier.particle_r;
+    p.alpha = tier.mote_a * std::pow(std::sin(kPi * age / life), 1.2f) *
+              (0.4f + 0.6f * Clamp01(w / 1.2f));
+    out->push_back(p);
+  }
+}
+
+float GlintLuminance(const EdgeLightTier& tier, float s, float t) {
+  constexpr float kSigma = 0.014f;  // 周长的 1.4%
+  float lum = 0;
+  for (int j = 0; j < tier.glints; ++j) {
+    const uint32_t u = static_cast<uint32_t>(j) + 5000;
+    const float life = 2.6f + 1.6f * Hash01(u, 1);
+    const float speed =
+        (0.010f + 0.008f * Hash01(u, 2)) * (Hash01(u, 3) < 0.5f ? -1.0f : 1.0f);
+    const float shifted = t + Hash01(u, 4) * life;
+    const float age = std::fmod(shifted, life);
+    const uint32_t round = static_cast<uint32_t>(std::floor(shifted / life));
+    const float center = Hash01(u, round, 5) + speed * age;
+    const float amp = std::pow(std::sin(kPi * age / life), 1.4f);
+    const float d = CircularDistance(s, center);
+    lum += amp * std::exp(-(d * d) / (2.0f * kSigma * kSigma));
+  }
+  // 只在浪高处亮。
+  return Clamp01(lum) * SmoothStep(0.5f, 0.8f, SilverWave(s, t));
+}
+
+float RippleLuminance(const EdgeLightTier& tier,
+                      float s,
+                      float source_s,
+                      float age) {
+  if (age < 0.0f) {
+    return 0.0f;
+  }
+  const float d = CircularDistance(s, source_s);
+  if (d > 0.45f) {
+    return 0.0f;
+  }
+  constexpr float kSigma = 0.02f;  // 周长的 2%
+  const float front = tier.ripple_v * age;
+  return std::exp(-age / 0.3f) *
+         std::exp(-((d - front) * (d - front)) / (2.0f * kSigma * kSigma));
+}
+
+// ---------------------------------------------------------------------------
+// 亮度分析
+// ---------------------------------------------------------------------------
+float EdgeLuminance::LightnessOfSegment(int segment) const {
+  if (!valid || point_luminance.empty()) {
+    return SmoothStep(0.35f, 0.8f, 0.95f);
+  }
+  const int begin = segment * kEdgeLightSegmentPoints;
+  const int end = std::min<int>(begin + kEdgeLightSegmentPoints,
+                                static_cast<int>(point_luminance.size()));
+  if (begin >= end) {
+    return SmoothStep(0.35f, 0.8f, overall);
+  }
+  float sum = 0;
+  for (int i = begin; i < end; ++i) {
+    sum += point_luminance[i];
+  }
+  return SmoothStep(0.35f, 0.8f, sum / static_cast<float>(end - begin));
+}
+
+float EdgeLuminance::OverallLightness() const {
+  return SmoothStep(0.35f, 0.8f, valid ? overall : 0.95f);
+}
+
+EdgeLuminance AnalyzeThumbnail(const SkBitmap& thumbnail,
+                               const EdgePerimeter& perimeter) {
+  EdgeLuminance result;
+  if (thumbnail.drawsNothing() || perimeter.empty() ||
+      perimeter.width() < 1 || perimeter.height() < 1) {
+    return result;
+  }
+  const int bw = thumbnail.width();
+  const int bh = thumbnail.height();
+  if (bw < 4 || bh < 4) {
+    return result;
+  }
+  const float web_w = perimeter.width();
+  const float web_h = perimeter.height();
+  const float sx = static_cast<float>(bw) / web_w;
+  const float sy = static_cast<float>(bh) / web_h;
+
+  std::vector<float> luma(static_cast<size_t>(bw) * bh);
+  for (int y = 0; y < bh; ++y) {
+    for (int x = 0; x < bw; ++x) {
+      luma[static_cast<size_t>(y) * bw + x] = Luma(thumbnail.getColor(x, y));
+    }
+  }
+  auto luma_at = [&](float wx, float wy) {
+    const int ix = std::clamp(static_cast<int>(wx * sx), 0, bw - 1);
+    const int iy = std::clamp(static_cast<int>(wy * sy), 0, bh - 1);
+    return luma[static_cast<size_t>(iy) * bw + ix];
+  };
+
+  // 贴边亮度：离边 6px 处取样，±6 点平滑。
+  const std::vector<EdgePoint>& pts = perimeter.points();
+  const int n = static_cast<int>(pts.size());
+  std::vector<float> raw(n);
+  for (int i = 0; i < n; ++i) {
+    raw[i] = luma_at(pts[i].x + pts[i].nx * 6.0f, pts[i].y + pts[i].ny * 6.0f);
+  }
+  result.point_luminance.resize(n);
+  float total = 0;
+  for (int i = 0; i < n; ++i) {
+    float sum = 0;
+    for (int k = -6; k <= 6; ++k) {
+      sum += raw[(i + k + n) % n];
+    }
+    result.point_luminance[i] = sum / 13.0f;
+    total += result.point_luminance[i];
+  }
+  result.overall = total / static_cast<float>(n);
+
+  // 文字遮罩：局部亮度跳变 > 0.16 的格子，外扩 8px，只留离边 4..70px 的。
+  const float cell_w = web_w / static_cast<float>(bw);
+  const float cell_h = web_h / static_cast<float>(bh);
+  std::vector<uint8_t> mark(static_cast<size_t>(bw) * bh, 0);
+  for (int y = 0; y < bh; ++y) {
+    for (int x = 0; x < bw; ++x) {
+      const float v = luma[static_cast<size_t>(y) * bw + x];
+      if (x + 1 < bw &&
+          std::fabs(v - luma[static_cast<size_t>(y) * bw + x + 1]) > 0.16f) {
+        mark[static_cast<size_t>(y) * bw + x] = 1;
+        mark[static_cast<size_t>(y) * bw + x + 1] = 1;
+      }
+      if (y + 1 < bh &&
+          std::fabs(v - luma[static_cast<size_t>(y + 1) * bw + x]) > 0.16f) {
+        mark[static_cast<size_t>(y) * bw + x] = 1;
+        mark[static_cast<size_t>(y + 1) * bw + x] = 1;
+      }
+    }
+  }
+  const int dil_x = std::max(1, static_cast<int>(std::ceil(8.0f / cell_w)));
+  const int dil_y = std::max(1, static_cast<int>(std::ceil(8.0f / cell_h)));
+  std::vector<uint8_t> tmp(mark.size(), 0);
+  for (int y = 0; y < bh; ++y) {  // 横向外扩
+    for (int x = 0; x < bw; ++x) {
+      if (!mark[static_cast<size_t>(y) * bw + x]) {
+        continue;
+      }
+      for (int k = -dil_x; k <= dil_x; ++k) {
+        const int xx = x + k;
+        if (xx >= 0 && xx < bw) {
+          tmp[static_cast<size_t>(y) * bw + xx] = 1;
+        }
+      }
+    }
+  }
+  std::vector<uint8_t> dil(mark.size(), 0);
+  for (int y = 0; y < bh; ++y) {  // 纵向外扩
+    for (int x = 0; x < bw; ++x) {
+      if (!tmp[static_cast<size_t>(y) * bw + x]) {
+        continue;
+      }
+      for (int k = -dil_y; k <= dil_y; ++k) {
+        const int yy = y + k;
+        if (yy >= 0 && yy < bh) {
+          dil[static_cast<size_t>(yy) * bw + x] = 1;
+        }
+      }
+    }
+  }
+  constexpr size_t kMaxRects = 800;
+  for (int y = 0; y < bh && result.text_rects.size() < kMaxRects; ++y) {
+    int x = 0;
+    while (x < bw && result.text_rects.size() < kMaxRects) {
+      auto keep = [&](int cx) {
+        const float wx = (static_cast<float>(cx) + 0.5f) * cell_w;
+        const float wy = (static_cast<float>(y) + 0.5f) * cell_h;
+        const float dist =
+            std::min(std::min(wx, web_w - wx), std::min(wy, web_h - wy));
+        return dil[static_cast<size_t>(y) * bw + cx] && dist >= 4.0f &&
+               dist <= 70.0f;
+      };
+      if (!keep(x)) {
+        ++x;
+        continue;
+      }
+      int x1 = x;
+      while (x1 + 1 < bw && keep(x1 + 1)) {
+        ++x1;
+      }
+      result.text_rects.push_back(SkRect::MakeLTRB(
+          static_cast<float>(x) * cell_w, static_cast<float>(y) * cell_h,
+          static_cast<float>(x1 + 1) * cell_w,
+          static_cast<float>(y + 1) * cell_h));
+      x = x1 + 1;
+    }
+  }
+  result.valid = true;
+  return result;
+}
+
+}  // namespace browseros::windrise
