diff --git a/chrome/browser/ui/views/tabs/vertical/top_container_button.cc b/chrome/browser/ui/views/tabs/vertical/top_container_button.cc
index 1029f4c..de5aa96 100644
--- a/chrome/browser/ui/views/tabs/vertical/top_container_button.cc
+++ b/chrome/browser/ui/views/tabs/vertical/top_container_button.cc
@@ -4,11 +4,16 @@
 
 #include "chrome/browser/ui/views/tabs/vertical/top_container_button.h"
 
+#include "cc/paint/paint_flags.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/color/chrome_color_id.h"
 #include "chrome/browser/ui/layout_constants.h"
 #include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
+#include "third_party/skia/include/core/SkRRect.h"
 #include "ui/base/metadata/metadata_impl_macros.h"
 #include "ui/compositor/layer.h"
+#include "ui/gfx/canvas.h"
+#include "ui/gfx/geometry/skia_conversions.h"
 #include "ui/views/actions/action_view_interface.h"
 #include "ui/views/widget/widget.h"
 
@@ -49,6 +54,24 @@ TopContainerButton::TopContainerButton() {
 
 TopContainerButton::~TopContainerButton() = default;
 
+void TopContainerButton::OnPaintBackground(gfx::Canvas* canvas) {
+  if (!GetColorProvider()) {
+    return;
+  }
+  // 方案标注表第 2 行：顶部小按钮 32 见方、底 #E8EBEE、圆角 10。颜色走 mixer
+  // 的 kColorWindriseTileBackground，圆角走偏好。
+  const float radius = static_cast<float>(
+      browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kTopButtonRadius));
+  cc::PaintFlags flags;
+  flags.setAntiAlias(true);
+  flags.setStyle(cc::PaintFlags::kFill_Style);
+  flags.setColor(GetColorProvider()->GetColor(kColorWindriseTileBackground));
+  canvas->sk_canvas()->drawRRect(
+      SkRRect::MakeRectXY(gfx::RectToSkRect(GetLocalBounds()), radius, radius),
+      flags);
+}
+
 void TopContainerButton::UpdateIcon(const ui::ImageModel& icon_image) {
   CHECK(icon_image.IsVectorIcon());
 
