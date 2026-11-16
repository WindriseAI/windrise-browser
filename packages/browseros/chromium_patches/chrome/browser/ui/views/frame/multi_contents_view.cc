diff --git a/chrome/browser/ui/views/frame/multi_contents_view.cc b/chrome/browser/ui/views/frame/multi_contents_view.cc
index a756f076ff..60a43d4d2e 100644
--- a/chrome/browser/ui/views/frame/multi_contents_view.cc
+++ b/chrome/browser/ui/views/frame/multi_contents_view.cc
@@ -9,6 +9,7 @@
 
 #include "base/check_deref.h"
 #include "base/notreached.h"
+#include "base/numerics/safe_conversions.h"
 #include "chrome/browser/actor/ui/actor_overlay_web_view.h"
 #include "chrome/browser/browser_process.h"
 #include "chrome/browser/profiles/profile.h"
@@ -41,12 +42,17 @@
 #include "components/tabs/public/tab_interface.h"
 #include "content/public/browser/web_contents.h"
 #include "content/public/common/url_constants.h"
+#include "ui/base/metadata/metadata_header_macros.h"
 #include "ui/base/metadata/metadata_impl_macros.h"
 #include "ui/base/ozone_buildflags.h"
+#include "ui/color/color_provider.h"
 #include "ui/compositor/layer.h"
+#include "ui/gfx/canvas.h"
 #include "ui/gfx/geometry/outsets.h"
 #include "ui/gfx/geometry/rect.h"
 #include "ui/gfx/geometry/rounded_corners_f.h"
+#include "ui/gfx/scoped_canvas.h"
+#include "ui/views/controls/separator.h"
 #include "ui/views/layout/flex_layout_types.h"
 #include "ui/views/layout/layout_types.h"
 #include "ui/views/layout/proposed_layout.h"
@@ -54,7 +60,51 @@
 
 namespace {
 constexpr int kSnapDistance = 15;
-}
+
+// 风桥 甲23 §4.2：网页上沿的分隔线照定稿帧 36:26925 画成 .5px #C9CED5。
+// 只画一个设备像素：1 倍屏上用 50% 透明度（帧里 y80 正是线色与网页各半的混色），
+// 2 倍及以上用实色（一个设备像素就是 .5px）。1.25–1.75 倍之间按同样的「墨量」
+// 取 min(1, 0.5 × 倍数)，这是本补丁的插值，裁定只定了 1 倍和 2 倍。
+// 线压在网页第一行上，不另占一行（见 CalculateSeparatorLayouts），所以它有自己的
+// layer，并且排在网页容器之后，画在网页上面；不接鼠标事件。
+class WindriseHairlineSeparator : public views::View {
+  METADATA_HEADER(WindriseHairlineSeparator, views::View)
+
+ public:
+  WindriseHairlineSeparator() {
+    SetPaintToLayer();
+    layer()->SetFillsBoundsOpaquely(false);
+    SetCanProcessEventsWithinSubtree(false);
+    SetPreferredSize(
+        gfx::Size(views::Separator::kThickness, views::Separator::kThickness));
+  }
+  WindriseHairlineSeparator(const WindriseHairlineSeparator&) = delete;
+  WindriseHairlineSeparator& operator=(const WindriseHairlineSeparator&) =
+      delete;
+  ~WindriseHairlineSeparator() override = default;
+
+  // views::View:
+  void OnPaint(gfx::Canvas* canvas) override {
+    const ui::ColorProvider* color_provider = GetColorProvider();
+    if (!color_provider) {
+      return;
+    }
+    gfx::ScopedCanvas scoped_canvas(canvas);
+    const float scale = canvas->UndoDeviceScaleFactor();
+    const SkColor color =
+        color_provider->GetColor(kColorToolbarContentAreaSeparator);
+    const float coverage = std::min(1.0f, 0.5f * scale);
+    canvas->FillRect(
+        gfx::Rect(0, 0, base::ClampCeil(width() * scale), 1),
+        SkColorSetA(color, base::ClampRound<U8CPU>(SkColorGetA(color) *
+                                                   coverage)));
+  }
+};
+
+BEGIN_METADATA(WindriseHairlineSeparator)
+END_METADATA
+
+}  // namespace
 
 void MultiContentsView::ContentsSeparators::Reset() {
   top_separator = nullptr;
@@ -98,8 +148,9 @@ MultiContentsView::MultiContentsView(
           *drop_target_view_, *delegate_, g_browser_process->local_state(),
           browser_view_->browser()->GetTabStripModel());
 
+  // 风桥 甲23 §4.2：顶边用 .5px 细线（见 WindriseHairlineSeparator）。
   contents_separators_.top_separator =
-      AddChildView(ContentsSeparator::CreateLayerBasedContentsSeparator());
+      AddChildView(std::make_unique<WindriseHairlineSeparator>());
   contents_separators_.top_separator->SetProperty(
       views::kElementIdentifierKey, kContentsSeparatorTopEdgeElementId);
 
@@ -217,15 +268,38 @@ ContentsContainerView* MultiContentsView::GetInactiveContentsContainerView()
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
@@ -507,8 +581,10 @@ views::ProposedLayout MultiContentsView::CalculateProposedLayout(
 
   gfx::Rect available_space = gfx::Rect(width, height);
 
-  const bool show_background =
-      drop_target_view_->GetVisible() || IsInSplitView();
+  // 风桥 N10：单窗口态网页区带圆角时也显示背景视图（窗口底色，不圆角），
+  // 圆角处露出的才是窗口底色而不是后面的工具栏色。
+  const bool show_background = drop_target_view_->GetVisible() ||
+                               IsInSplitView() || !background_radii_.IsEmpty();
   layouts.child_layouts.emplace_back(background_view_.get(), show_background,
                                      available_space);
 
@@ -725,10 +801,12 @@ gfx::Rect MultiContentsView::CalculateSeparatorLayouts(
   }
   child_layouts.push_back(corner_layout);
 
+  // 风桥 甲23 §4.4：顶边细线压在网页第一行上（定稿帧 36:26925 里线直接压在网页
+  // 上沿，不另占一行），网页不再为它往下让 1px。
   return gfx::Rect(available_space.x() + leading_separator_width,
-                   available_space.y() + separator_height,
+                   available_space.y(),
                    width - trailing_separator_width - leading_separator_width,
-                   height - separator_height);
+                   height);
 }
 
 MultiContentsView::ViewSizes MultiContentsView::GetViewSizes(
@@ -796,6 +874,7 @@ int MultiContentsView::GetMinViewSize(gfx::Rect available_space) const {
 
 void MultiContentsView::UpdateContentsBorderAndOverlay() {
   const bool is_in_split = IsInSplitView();
+  UpdateBackgroundView();
   for (auto& contents_container_view : contents_container_views_) {
     const bool is_active =
         contents_container_view->contents_view() == GetActiveContentsView();
