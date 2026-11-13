diff --git a/chrome/browser/ui/views/frame/multi_contents_background_view.cc b/chrome/browser/ui/views/frame/multi_contents_background_view.cc
index 56e57faa10..ca07f320b5 100644
--- a/chrome/browser/ui/views/frame/multi_contents_background_view.cc
+++ b/chrome/browser/ui/views/frame/multi_contents_background_view.cc
@@ -4,8 +4,10 @@
 
 #include "chrome/browser/ui/views/frame/multi_contents_background_view.h"
 
+#include "chrome/browser/ui/color/chrome_color_id.h"
 #include "chrome/browser/ui/views/frame/themed_background.h"
 #include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/color/color_provider.h"
 #include "ui/compositor/layer_solid_color.h"
 #include "ui/gfx/geometry/rounded_corners_f.h"
 
@@ -28,10 +30,27 @@ const gfx::RoundedCornersF& MultiContentsBackgroundView::GetRoundedCorners()
   return layer()->rounded_corner_radii();
 }
 
+void MultiContentsBackgroundView::SetUseWindowGapColor(bool use) {
+  if (use_window_gap_color_ == use) {
+    return;
+  }
+  use_window_gap_color_ = use;
+  if (GetWidget()) {
+    RefreshLayerAndColor();
+  }
+}
+
 void MultiContentsBackgroundView::OnThemeChanged() {
   views::View::OnThemeChanged();
+  RefreshLayerAndColor();
+}
+
+void MultiContentsBackgroundView::RefreshLayerAndColor() {
   if (auto new_type = CalculateLayerType(); new_type != layer()->type()) {
+    // 换层类型会重建 layer，圆角要带过去。
+    const gfx::RoundedCornersF radii = layer()->rounded_corner_radii();
     SetPaintToLayer(new_type);
+    SetRoundedCorners(radii);
   }
 
   if (layer()->AsSolidColor()) {
@@ -47,6 +66,9 @@ void MultiContentsBackgroundView::OnPaint(gfx::Canvas* canvas) {
 }
 
 ui::LayerType MultiContentsBackgroundView::CalculateLayerType() const {
+  if (use_window_gap_color_) {
+    return ui::LAYER_SOLID_COLOR;
+  }
   const bool has_custom_image =
       !ThemedBackground::GetBackgroundColor(this, browser_view_).has_value();
   return has_custom_image ? ui::LAYER_TEXTURED : ui::LAYER_SOLID_COLOR;
@@ -54,6 +76,13 @@ ui::LayerType MultiContentsBackgroundView::CalculateLayerType() const {
 
 void MultiContentsBackgroundView::UpdateSolidLayerColor() {
   CHECK(layer()->AsSolidColor());
+  if (use_window_gap_color_) {
+    if (const ui::ColorProvider* provider = GetColorProvider()) {
+      layer()->AsSolidColor()->SetColor(
+          SkColor4f::FromColor(provider->GetColor(kColorWindriseWebContentsGap)));
+    }
+    return;
+  }
   if (auto color = ThemedBackground::GetBackgroundColor(this, browser_view_)) {
     layer()->AsSolidColor()->SetColor(SkColor4f::FromColor(*color));
   }
