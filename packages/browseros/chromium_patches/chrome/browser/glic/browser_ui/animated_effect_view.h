diff --git a/chrome/browser/glic/browser_ui/animated_effect_view.h b/chrome/browser/glic/browser_ui/animated_effect_view.h
index 7230bc1d0b..1087f32307 100644
--- a/chrome/browser/glic/browser_ui/animated_effect_view.h
+++ b/chrome/browser/glic/browser_ui/animated_effect_view.h
@@ -91,6 +91,14 @@ class AnimatedEffectView : public views::View,
   // Returns the colors to be used for the shader effect.
   virtual std::vector<SkColor> GetEffectColors();
 
+  // 风桥 N11：子类可以换自己的着色器资源（默认是 glic 的边框着色器）。
+  // `simplified` 为 true 表示 GPU 不可用 / 减少动态效果时的简化版本。
+  virtual int GetShaderResourceId(bool simplified) const;
+
+  // 风桥 N11：起 / 收的时长。默认 500ms / 200ms（上游值）。
+  virtual base::TimeDelta GetRampUpDuration() const;
+  virtual base::TimeDelta GetRampDownDuration() const;
+
   // Sets the shader uniforms for the given effect.
   virtual void PopulateShaderUniforms(
       std::vector<cc::PaintShader::FloatUniform>& float_uniforms,
