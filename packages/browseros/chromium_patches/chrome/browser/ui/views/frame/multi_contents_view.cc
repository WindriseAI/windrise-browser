diff --git a/chrome/browser/ui/views/frame/multi_contents_view.cc b/chrome/browser/ui/views/frame/multi_contents_view.cc
index a756f076ff..7b5c9090ec 100644
--- a/chrome/browser/ui/views/frame/multi_contents_view.cc
+++ b/chrome/browser/ui/views/frame/multi_contents_view.cc
@@ -217,15 +217,38 @@ ContentsContainerView* MultiContentsView::GetInactiveContentsContainerView()
 }
 
 const gfx::RoundedCornersF& MultiContentsView::GetBackgroundRadii() const {
-  return background_view_->GetRoundedCorners();
+  return background_radii_;
+}
+
+void MultiContentsView::UpdateBackgroundView() {
+  const bool split = IsInSplitView();
+  // 分屏：和上游一样，背景视图带圆角、用工具栏色做两个网页之间的缝。
+  // 单窗口：背景视图不加圆角，用窗口底色填满，露在圆角网页区的角上。
+  const gfx::RoundedCornersF applied =
+      split ? background_radii_ : gfx::RoundedCornersF();
+  bool changed = false;
+  if (!(background_view_->GetRoundedCorners() == applied)) {
+    background_view_->SetRoundedCorners(applied);
+    changed = true;
+  }
+  const bool want_gap_mode = !split;
+  if (background_gap_mode_ != want_gap_mode) {
+    background_gap_mode_ = want_gap_mode;
+    background_view_->SetUseWindowGapColor(want_gap_mode);
+    changed = true;
+  }
+  if (changed) {
+    InvalidateLayout(/*avoid_propagate_during_layout=*/true);
+  }
 }
 
 void MultiContentsView::SetBackgroundRadii(const gfx::RoundedCornersF& radii) {
-  if (radii == GetBackgroundRadii()) {
+  if (radii == background_radii_) {
     return;
   }
 
-  background_view_->SetRoundedCorners(radii);
+  background_radii_ = radii;
+  UpdateBackgroundView();
 
   if (!IsInSplitView()) {
     GetActiveContentsContainerView()->SetRoundedCorners(radii);
@@ -507,8 +530,10 @@ views::ProposedLayout MultiContentsView::CalculateProposedLayout(
 
   gfx::Rect available_space = gfx::Rect(width, height);
 
-  const bool show_background =
-      drop_target_view_->GetVisible() || IsInSplitView();
+  // 风桥 N10：单窗口态网页区带圆角时也显示背景视图（窗口底色，不圆角），
+  // 圆角处露出的才是窗口底色而不是后面的工具栏色。
+  const bool show_background = drop_target_view_->GetVisible() ||
+                               IsInSplitView() || !background_radii_.IsEmpty();
   layouts.child_layouts.emplace_back(background_view_.get(), show_background,
                                      available_space);
 
@@ -796,6 +821,7 @@ int MultiContentsView::GetMinViewSize(gfx::Rect available_space) const {
 
 void MultiContentsView::UpdateContentsBorderAndOverlay() {
   const bool is_in_split = IsInSplitView();
+  UpdateBackgroundView();
   for (auto& contents_container_view : contents_container_views_) {
     const bool is_active =
         contents_container_view->contents_view() == GetActiveContentsView();
