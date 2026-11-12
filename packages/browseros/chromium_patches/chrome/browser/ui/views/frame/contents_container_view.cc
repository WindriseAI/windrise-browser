diff --git a/chrome/browser/ui/views/frame/contents_container_view.cc b/chrome/browser/ui/views/frame/contents_container_view.cc
index 413b2f6..c443251 100644
--- a/chrome/browser/ui/views/frame/contents_container_view.cc
+++ b/chrome/browser/ui/views/frame/contents_container_view.cc
@@ -10,6 +10,8 @@
 
 #include "base/i18n/rtl.h"
 #include "chrome/browser/actor/ui/actor_overlay_web_view.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
 #include "chrome/browser/enterprise/data_protection/data_protection_overlay_view.h"
 #include "chrome/browser/glic/browser_ui/context_sharing_border_view.h"
@@ -20,11 +22,13 @@
 #include "chrome/browser/ui/read_anything/read_anything_immersive_overlay_view.h"
 #include "chrome/browser/ui/ui_features.h"
 #include "chrome/browser/ui/view_ids.h"
+#include "chrome/browser/extensions/extension_tab_util.h"
 #include "chrome/browser/ui/views/frame/browser_view.h"
 #include "chrome/browser/ui/views/frame/contents_capture_border_view.h"
 #include "chrome/browser/ui/views/frame/contents_container_outline.h"
 #include "chrome/browser/ui/views/frame/contents_separator.h"
 #include "chrome/browser/ui/views/frame/contents_web_view.h"
+#include "chrome/browser/ui/views/frame/windrise_silver_wave_view.h"
 #include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
 #include "chrome/browser/ui/views/frame/scrim_view.h"
 #include "chrome/browser/ui/views/frame/tab_modal_dialog_host.h"
@@ -156,6 +160,16 @@ ContentsContainerView::ContentsContainerView(BrowserView* browser_view)
             .Build());
   }
 
+  // 风桥 N11：银色海浪层，画在网页区上方、外扩 silver_wave_spread 一圈。
+  silver_wave_view_ = AddChildView(std::make_unique<WindriseSilverWaveView>(
+      browser_view->GetProfile()));
+  silver_wave_view_->SetCanProcessEventsWithinSubtree(false);
+  if (browseros::windrise::AiWindowState* ai_state =
+          browseros::windrise::AiWindowState::Attach(browser_view->browser())) {
+    ai_state_subscription_ = ai_state->AddObserver(base::BindRepeating(
+        &ContentsContainerView::UpdateSilverWave, base::Unretained(this)));
+  }
+
   mini_toolbar_ = AddChildView(std::make_unique<MultiContentsViewMiniToolbar>(
       browser_view, contents_view_));
 
@@ -166,9 +180,15 @@ ContentsContainerView::ContentsContainerView(BrowserView* browser_view)
       AddChildView(std::make_unique<ContentsCaptureBorderView>(mini_toolbar_));
 
   view_bounds_observer_.Observe(contents_view_);
+  UpdateSilverWave();
 }
 
 ContentsContainerView::~ContentsContainerView() {
+  ai_state_subscription_ = {};
+  if (browser_view_) {
+    browseros::windrise::AiWindowState::Detach(browser_view_->browser());
+  }
+  silver_wave_view_ = nullptr;
   indigo_overlay_view_ = nullptr;
   // read_anything_immersive_overlay_view_ holds a raw_ptr to
   // contents_view_. We need to make sure we destroy
@@ -318,6 +338,9 @@ void ContentsContainerView::ChildVisibilityChanged(View* child) {
 }
 
 void ContentsContainerView::Layout(PassKey pass_key) {
+  // 切标签时网页区的 bounds 不一定变，但显示的是另一个标签；每次布局顺带按
+  // 当前 WebContents 的 AI 状态同步一次开 / 关（幂等，不改布局）。
+  SyncSilverWaveVisibility();
   LayoutSuperclass<views::View>(this);
 
   UpdateContentsClip();
@@ -341,7 +364,38 @@ void ContentsContainerView::OnViewBoundsChanged(View* observed_view) {
     if (!rounded_corner_radii_.IsEmpty()) {
       UpdateBorderRoundedCorners();
     }
+    UpdateSilverWave();
+  }
+}
+
+void ContentsContainerView::UpdateSilverWave() {
+  if (!silver_wave_view_) {
+    return;
   }
+  // 圆角：上两角是网页区圆角，下两角跟窗口（Windows 11 由系统裁，最大化时 0）。
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const float top = static_cast<float>(
+      metrics.Get(browseros::windrise::LayoutMetric::kWebContentsRadius));
+  const float bottom = static_cast<float>(
+      metrics.Get(browseros::windrise::LayoutMetric::kWindowCornerRadius));
+  silver_wave_view_->SetCornerRadii(
+      gfx::RoundedCornersF(top, top, bottom, bottom));
+
+  SyncSilverWaveVisibility();
+  InvalidateLayout();
+}
+
+void ContentsContainerView::SyncSilverWaveVisibility() {
+  if (!silver_wave_view_) {
+    return;
+  }
+  browseros::windrise::AiWindowState* ai_state =
+      browseros::windrise::AiWindowState::From(browser_view_->browser());
+  content::WebContents* web_contents = contents_view_->web_contents();
+  const int tab_id =
+      web_contents ? extensions::ExtensionTabUtil::GetTabId(web_contents) : 0;
+  silver_wave_view_->SetShowing(ai_state && tab_id && ai_state->IsActive(tab_id));
 }
 
 void ContentsContainerView::SetContentsResizingStrategy(
@@ -537,6 +591,16 @@ views::ProposedLayout ContentsContainerView::CalculateProposedLayout(
                                        non_devtools_contents_bounds);
   }
 
+  if (silver_wave_view_) {
+    // 风桥 N11：本层 = 网页区向外扩 Spread（外溢辉光画到标签栏 / 工具栏挨着的
+    // 那一条）。不接输入；内部按距离场自己画圆角。
+    const int spread = WindriseSilverWaveView::GetSpread();
+    gfx::Rect wave_bounds = contents_rect;
+    wave_bounds.Inset(gfx::Insets::VH(-spread, -spread));
+    layouts.child_layouts.emplace_back(
+        silver_wave_view_.get(), silver_wave_view_->IsShowing(), wave_bounds);
+  }
+
   // The content scrim view should cover the entire contents bounds.
   CHECK(contents_scrim_view_);
   layouts.child_layouts.emplace_back(contents_scrim_view_.get(),
