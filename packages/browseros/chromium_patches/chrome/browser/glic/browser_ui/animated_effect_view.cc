diff --git a/chrome/browser/glic/browser_ui/animated_effect_view.cc b/chrome/browser/glic/browser_ui/animated_effect_view.cc
index b366e3e20b..ca198ef5a6 100644
--- a/chrome/browser/glic/browser_ui/animated_effect_view.cc
+++ b/chrome/browser/glic/browser_ui/animated_effect_view.cc
@@ -344,7 +344,8 @@ std::vector<SkColor> AnimatedEffectView::GetEffectColors() {
 
 float AnimatedEffectView::GetOpacity(base::TimeTicks timestamp) {
   auto ramp_up_duration = skip_animation_cycle_ ? kFastOpacityRampUpDuration
-                                                : kOpacityRampUpDuration;
+                                                : GetRampUpDuration();
+  const base::TimeDelta ramp_down_duration = GetRampDownDuration();
   if (!first_ramp_down_frame_.is_null()) {
     // The ramp up opacity could be any value between 0-1 during the ramp up
     // time. Thus, the ramping down opacity must be deducted from the value of
@@ -359,7 +360,7 @@ float AnimatedEffectView::GetOpacity(base::TimeTicks timestamp) {
         timestamp - first_ramp_down_frame_;
     float ramp_down_opacity =
         static_cast<float>(time_since_first_ramp_down_frame.InMillisecondsF() /
-                           kOpacityRampDownDuration.InMillisecondsF());
+                           ramp_down_duration.InMillisecondsF());
     ramp_down_opacity_ =
         std::clamp(ramp_up_opacity - ramp_down_opacity, 0.0f, 1.0f);
     return ramp_down_opacity_;
@@ -444,15 +445,24 @@ void AnimatedEffectView::UpdateShader() {
             : ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
                   IDR_GLIC_PARAMETERIZED_BORDER_SHADER);
   } else {
-    shader_ =
-        ForceSimplifiedShader()
-            ? ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
-                  IDR_GLIC_SIMPLIFIED_BORDER_SHADER)
-            : ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
-                  IDR_GLIC_BORDER_SHADER);
+    shader_ = ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
+        GetShaderResourceId(ForceSimplifiedShader()));
   }
 }
 
+int AnimatedEffectView::GetShaderResourceId(bool simplified) const {
+  return simplified ? IDR_GLIC_SIMPLIFIED_BORDER_SHADER
+                    : IDR_GLIC_BORDER_SHADER;
+}
+
+base::TimeDelta AnimatedEffectView::GetRampUpDuration() const {
+  return kOpacityRampUpDuration;
+}
+
+base::TimeDelta AnimatedEffectView::GetRampDownDuration() const {
+  return kOpacityRampDownDuration;
+}
+
 void AnimatedEffectView::SetDefaultColors(cc::PaintFlags& paint_flags,
                                           const gfx::RectF& bounds) const {
 #if BUILDFLAG(GOOGLE_CHROME_BRANDING)
