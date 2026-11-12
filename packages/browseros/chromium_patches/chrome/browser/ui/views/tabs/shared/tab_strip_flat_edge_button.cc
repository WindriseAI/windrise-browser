diff --git a/chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.cc b/chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.cc
index d53de82..c492e5e 100644
--- a/chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.cc
+++ b/chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.cc
@@ -284,13 +284,44 @@ void TabStripFlatEdgeButton::RemovedFromWidget() {
   paint_as_active_subscription_ = {};
 }
 
+void TabStripFlatEdgeButton::SetBackgroundColorId(
+    std::optional<ui::ColorId> color_id) {
+  if (background_color_id_ == color_id) {
+    return;
+  }
+  background_color_id_ = color_id;
+  SchedulePaint();
+}
+
+void TabStripFlatEdgeButton::SetForegroundColorId(
+    std::optional<ui::ColorId> color_id) {
+  if (foreground_color_id_ == color_id) {
+    return;
+  }
+  foreground_color_id_ = color_id;
+  const std::optional<ui::ImageModel>& model =
+      GetImageModel(views::Button::STATE_NORMAL);
+  if (model && model->IsVectorIcon()) {
+    UpdateIcon(*model);
+  }
+  if (should_show_label_) {
+    UpdateLabelColor();
+  }
+}
+
 ui::ColorId TabStripFlatEdgeButton::GetForegroundColor() const {
+  if (foreground_color_id_) {
+    return *foreground_color_id_;
+  }
   return GetWidget() && GetWidget()->ShouldPaintAsActive()
              ? kColorTabSearchButtonCRForegroundFrameActive
              : kColorTabSearchButtonCRForegroundFrameInactive;
 }
 
 ui::ColorId TabStripFlatEdgeButton::GetBackgroundColor() const {
+  if (background_color_id_) {
+    return *background_color_id_;
+  }
   return GetWidget() && GetWidget()->ShouldPaintAsActive()
              ? kColorNewTabButtonCRBackgroundFrameActive
              : kColorNewTabButtonCRBackgroundFrameInactive;
