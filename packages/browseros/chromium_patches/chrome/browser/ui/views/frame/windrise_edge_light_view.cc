diff --git a/chrome/browser/ui/views/frame/windrise_edge_light_view.cc b/chrome/browser/ui/views/frame/windrise_edge_light_view.cc
new file mode 100644
--- /dev/null
+++ b/chrome/browser/ui/views/frame/windrise_edge_light_view.cc
@@ -0,0 +1,52 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+//
+// 11-13 编译摘除：N11 原生 Skia 边缘光效只作备用（浏览器里的光效改由网页代码画，
+// 见 edge-light-web 线）。这一版它有多处与 158.0.8127.41 不兼容的报错
+// （base::NoDestructor 静态断言、viz::CopyOutputBitmapWithMetadata 前置声明不全），
+// 按协调人 11-13 的决定把视图换成不画任何东西的空实现：对外接口不变，
+// ContentsContainerView 等引用端照常编译。不画光、不做动画、不取缩略图。
+
+#include "chrome/browser/ui/views/frame/windrise_edge_light_view.h"
+
+#include "ui/base/metadata/metadata_impl_macros.h"
+
+WindriseEdgeLightView::WindriseEdgeLightView() = default;
+WindriseEdgeLightView::~WindriseEdgeLightView() = default;
+
+void WindriseEdgeLightView::SetShowing(bool showing) {
+  wants_showing_ = showing;
+  phase_ = showing ? Phase::kSteady : Phase::kHidden;
+}
+
+void WindriseEdgeLightView::SetCornerRadii(const gfx::RoundedCornersF& radii) {
+  corner_radii_ = radii;
+}
+
+void WindriseEdgeLightView::SetCornerProgressCallback(
+    CornerProgressCallback callback) {}
+
+void WindriseEdgeLightView::SetWebContents(
+    content::WebContents* web_contents) {}
+
+void WindriseEdgeLightView::Pulse(
+    const browseros::windrise::AiPulse& pulse) {}
+
+void WindriseEdgeLightView::AddedToWidget() {}
+
+void WindriseEdgeLightView::RemovedFromWidget() {}
+
+void WindriseEdgeLightView::OnPaint(gfx::Canvas* canvas) {}
+
+void WindriseEdgeLightView::OnAnimationStep(base::TimeTicks timestamp) {}
+
+void WindriseEdgeLightView::OnCompositingShuttingDown(
+    ui::Compositor* compositor) {}
+
+void WindriseEdgeLightView::OnGpuInfoUpdate() {}
+
+void WindriseEdgeLightView::PaintFrame(gfx::Canvas* canvas,
+                                       const FrameState& frame) {}
+
+BEGIN_METADATA(WindriseEdgeLightView)
+END_METADATA
