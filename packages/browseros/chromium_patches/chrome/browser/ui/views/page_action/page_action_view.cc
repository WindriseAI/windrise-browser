diff --git a/chrome/browser/ui/views/page_action/page_action_view.cc b/chrome/browser/ui/views/page_action/page_action_view.cc
index ad4c332e03..b199f5d4a1 100644
--- a/chrome/browser/ui/views/page_action/page_action_view.cc
+++ b/chrome/browser/ui/views/page_action/page_action_view.cc
@@ -739,6 +739,11 @@ void PageActionView::MaybeRecordCollapsedMetrics(int label_width) {
 }
 
 SkColor PageActionView::GetBackgroundColor() const {
+  // 风桥闪办芯片：墨色实心。颜色来自 windrise color mixer，读
+  // browseros.windrise.theme.* 偏好，运行时可改。
+  if (type_ == PageActionIconType::kWindriseSlot && GetColorProvider()) {
+    return GetColorProvider()->GetColor(kColorWindriseSlotChipBackground);
+  }
   if (observation_.IsObserving() &&
       observation_.GetSource()->GetOverrideBackgroundColorId().has_value() &&
       GetColorProvider()) {
@@ -749,6 +754,11 @@ SkColor PageActionView::GetBackgroundColor() const {
 }
 
 SkColor PageActionView::GetForegroundColor() const {
+  // 同上：芯片的字与闪电在墨色底上用白色。图标取 GetForegroundColor()，
+  // 因为 page action 的 color source 默认是 kForeground。
+  if (type_ == PageActionIconType::kWindriseSlot && GetColorProvider()) {
+    return GetColorProvider()->GetColor(kColorWindriseSlotChipForeground);
+  }
   if (observation_.IsObserving() &&
       observation_.GetSource()->GetOverrideForegroundColorId().has_value() &&
       GetColorProvider()) {
