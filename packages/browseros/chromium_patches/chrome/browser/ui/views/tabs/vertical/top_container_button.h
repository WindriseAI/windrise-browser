diff --git a/chrome/browser/ui/views/tabs/vertical/top_container_button.h b/chrome/browser/ui/views/tabs/vertical/top_container_button.h
index 87dd1fb..65bd8cf 100644
--- a/chrome/browser/ui/views/tabs/vertical/top_container_button.h
+++ b/chrome/browser/ui/views/tabs/vertical/top_container_button.h
@@ -25,6 +25,8 @@ class TopContainerButton : public views::LabelButton {
   gfx::Size CalculatePreferredSize(
       const views::SizeBounds& available_size) const override;
   std::unique_ptr<views::ActionViewInterface> GetActionViewInterface() override;
+  // 风桥 N3：顶部小按钮的 tile 底 + 圆角（颜色 / 圆角都来自外观偏好）。
+  void OnPaintBackground(gfx::Canvas* canvas) override;
 
  private:
   // views::View:
