diff --git a/ui/views/bubble/bubble_dialog_delegate_view.cc b/ui/views/bubble/bubble_dialog_delegate_view.cc
index 53e9071..15254ab 100644
--- a/ui/views/bubble/bubble_dialog_delegate_view.cc
+++ b/ui/views/bubble/bubble_dialog_delegate_view.cc
@@ -484,6 +484,11 @@ BubbleDialogDelegate::BubbleDialogDelegate(BubbleAnchor anchor,
   SetShowCloseButton(false);
 
   LayoutProvider* const layout_provider = LayoutProvider::Get();
+  // 风桥 N2：气泡圆角独立于对话框。对话框走 kDialogRadius（ChromeLayoutProvider
+  // 把它映射到 radius_large = 16），气泡在这里固定成菜单 / 气泡档
+  // （radius_medium = 12）。派生类里显式 set_corner_radius 的仍然覆盖这里。
+  set_corner_radius(
+      layout_provider->GetCornerRadiusMetric(views::kMenuRadius));
   set_frame_margins(
       {.contents = layout_provider->GetDialogInsetsForContentType(
            DialogContentType::kText, DialogContentType::kText),
