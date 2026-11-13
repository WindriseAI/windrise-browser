diff --git a/chrome/browser/ui/views/frame/contents_container_view.cc b/chrome/browser/ui/views/frame/contents_container_view.cc
index 413b2f626d..578e40cfd4 100644
--- a/chrome/browser/ui/views/frame/contents_container_view.cc
+++ b/chrome/browser/ui/views/frame/contents_container_view.cc
@@ -4,12 +4,15 @@
 
 #include "chrome/browser/ui/views/frame/contents_container_view.h"
 
+#include <algorithm>
 #include <memory>
 #include <optional>
 #include <utility>
 
 #include "base/i18n/rtl.h"
 #include "chrome/browser/actor/ui/actor_overlay_web_view.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
 #include "chrome/browser/enterprise/data_protection/data_protection_overlay_view.h"
 #include "chrome/browser/glic/browser_ui/context_sharing_border_view.h"
@@ -20,11 +23,13 @@
 #include "chrome/browser/ui/read_anything/read_anything_immersive_overlay_view.h"
 #include "chrome/browser/ui/ui_features.h"
 #include "chrome/browser/ui/view_ids.h"
+#include "chrome/browser/extensions/extension_tab_util.h"
 #include "chrome/browser/ui/views/frame/browser_view.h"
 #include "chrome/browser/ui/views/frame/contents_capture_border_view.h"
 #include "chrome/browser/ui/views/frame/contents_container_outline.h"
 #include "chrome/browser/ui/views/frame/contents_separator.h"
 #include "chrome/browser/ui/views/frame/contents_web_view.h"
+#include "chrome/browser/ui/views/frame/windrise_edge_light_view.h"
 #include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
 #include "chrome/browser/ui/views/frame/scrim_view.h"
 #include "chrome/browser/ui/views/frame/tab_modal_dialog_host.h"
@@ -156,6 +161,23 @@ ContentsContainerView::ContentsContainerView(BrowserView* browser_view)
             .Build());
   }
 
+  // 风桥 N11：边缘光效层，画在网页区上方（层 = 网页区，外扩 0），不接输入。
+  // N10：网页角 12 -> 36 的过渡进度由它按合成器时间戳算，经回调交给本类。
+  edge_light_view_ = AddChildView(std::make_unique<WindriseEdgeLightView>());
+  edge_light_view_->SetCanProcessEventsWithinSubtree(false);
+  edge_light_view_->SetCornerProgressCallback(base::BindRepeating(
+      &ContentsContainerView::OnAiCornerProgress, base::Unretained(this)));
+  // 析构时 BrowserView 的成员可能已经先拆掉了，所以这里把窗口指针存下来，
+  // 析构只用它做 Detach 的键。
+  windrise_ai_window_ = browser_view->browser();
+  if (browseros::windrise::AiWindowState* ai_state =
+          browseros::windrise::AiWindowState::Attach(windrise_ai_window_)) {
+    ai_state_subscription_ = ai_state->AddObserver(base::BindRepeating(
+        &ContentsContainerView::UpdateEdgeLight, base::Unretained(this)));
+    ai_pulse_subscription_ = ai_state->AddPulseObserver(base::BindRepeating(
+        &ContentsContainerView::OnAiPulse, base::Unretained(this)));
+  }
+
   mini_toolbar_ = AddChildView(std::make_unique<MultiContentsViewMiniToolbar>(
       browser_view, contents_view_));
 
@@ -166,9 +188,18 @@ ContentsContainerView::ContentsContainerView(BrowserView* browser_view)
       AddChildView(std::make_unique<ContentsCaptureBorderView>(mini_toolbar_));
 
   view_bounds_observer_.Observe(contents_view_);
+  UpdateEdgeLight();
 }
 
 ContentsContainerView::~ContentsContainerView() {
+  ai_state_subscription_ = {};
+  ai_pulse_subscription_ = {};
+  if (edge_light_view_) {
+    // 子视图在基类析构里才删，回调不能再指向已析构的本类。
+    edge_light_view_->SetCornerProgressCallback({});
+  }
+  browseros::windrise::AiWindowState::Detach(windrise_ai_window_);
+  edge_light_view_ = nullptr;
   indigo_overlay_view_ = nullptr;
   // read_anything_immersive_overlay_view_ holds a raw_ptr to
   // contents_view_. We need to make sure we destroy
@@ -304,10 +335,47 @@ void ContentsContainerView::SetBorderRoundedCornersFrom(
   if (glic_border_) {
     glic_border_->SetRoundedCorners(content_rounded_corners);
   }
+
+  if (edge_light_view_) {
+    // 光的外沿贴着网页角：同一组数（含 N10 的 AI 过渡）。
+    edge_light_view_->SetCornerRadii(content_rounded_corners);
+  }
 }
 
 void ContentsContainerView::UpdateBorderRoundedCorners() {
-  SetBorderRoundedCornersFrom(rounded_corner_radii_);
+  last_effective_radii_ = GetEffectiveRoundedCorners();
+  SetBorderRoundedCornersFrom(last_effective_radii_);
+}
+
+gfx::RoundedCornersF ContentsContainerView::GetEffectiveRoundedCorners() const {
+  if (ai_corner_progress_ <= 0.0f || rounded_corner_radii_.IsEmpty() ||
+      is_in_split_ || browser_view_->IsFullscreen()) {
+    return rounded_corner_radii_;
+  }
+  content::WebContents* web_contents = contents_view_->web_contents();
+  if (web_contents && web_contents->IsFullscreen()) {
+    return rounded_corner_radii_;  // HTML5 元素全屏：不做 36
+  }
+  const float target = static_cast<float>(
+      browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kWebContentsRadiusAi));
+  const float p = std::clamp(ai_corner_progress_, 0.0f, 1.0f);
+  auto lerp = [&](float base) { return base + (target - base) * p; };
+  // 逐角合成：下两角基准是 0（非玻璃，由窗口圆角裁）就从 0 过渡到 36。
+  return gfx::RoundedCornersF(lerp(rounded_corner_radii_.upper_left()),
+                              lerp(rounded_corner_radii_.upper_right()),
+                              lerp(rounded_corner_radii_.lower_right()),
+                              lerp(rounded_corner_radii_.lower_left()));
+}
+
+void ContentsContainerView::OnAiCornerProgress(float progress) {
+  ai_corner_progress_ = progress;
+  if (GetEffectiveRoundedCorners() == last_effective_radii_) {
+    return;
+  }
+  // 逐帧应用：devtools 停靠侧 / 新标签页 footer 的角由 SetBorderRoundedCornersFrom
+  // 逐角置 0 的逻辑原样生效。
+  UpdateBorderRoundedCorners();
 }
 
 void ContentsContainerView::ChildVisibilityChanged(View* child) {
@@ -318,6 +386,9 @@ void ContentsContainerView::ChildVisibilityChanged(View* child) {
 }
 
 void ContentsContainerView::Layout(PassKey pass_key) {
+  // 切标签时网页区的 bounds 不一定变，但显示的是另一个标签；每次布局顺带按
+  // 当前 WebContents 的 AI 状态同步一次开 / 关（幂等，不改布局）。
+  SyncEdgeLight();
   LayoutSuperclass<views::View>(this);
 
   UpdateContentsClip();
@@ -341,9 +412,41 @@ void ContentsContainerView::OnViewBoundsChanged(View* observed_view) {
     if (!rounded_corner_radii_.IsEmpty()) {
       UpdateBorderRoundedCorners();
     }
+    UpdateEdgeLight();
   }
 }
 
+void ContentsContainerView::UpdateEdgeLight() {
+  if (!edge_light_view_) {
+    return;
+  }
+  SyncEdgeLight();
+  InvalidateLayout();
+}
+
+void ContentsContainerView::SyncEdgeLight() {
+  if (!edge_light_view_) {
+    return;
+  }
+  browseros::windrise::AiWindowState* ai_state =
+      browseros::windrise::AiWindowState::From(browser_view_->browser());
+  content::WebContents* web_contents = contents_view_->web_contents();
+  const int tab_id =
+      web_contents ? extensions::ExtensionTabUtil::GetTabId(web_contents) : 0;
+  edge_light_view_->SetWebContents(web_contents);
+  edge_light_view_->SetShowing(ai_state && tab_id && ai_state->IsActive(tab_id));
+}
+
+void ContentsContainerView::OnAiPulse(
+    const browseros::windrise::AiPulse& pulse) {
+  content::WebContents* web_contents = contents_view_->web_contents();
+  if (!edge_light_view_ || !web_contents ||
+      extensions::ExtensionTabUtil::GetTabId(web_contents) != pulse.tab_id) {
+    return;
+  }
+  edge_light_view_->Pulse(pulse);
+}
+
 void ContentsContainerView::SetContentsResizingStrategy(
     const DevToolsContentsResizingStrategy& strategy) {
   if (strategy_.Equals(strategy)) {
@@ -537,6 +640,15 @@ views::ProposedLayout ContentsContainerView::CalculateProposedLayout(
                                        non_devtools_contents_bounds);
   }
 
+  if (edge_light_view_) {
+    // 风桥 N11：层 = 网页区（外扩 0），和 glic 边框同一个挂点。可见性由光效层自己
+    // 管（淡出结束才隐藏），这里不能用 IsShowing()：开始淡出那一刻它就变 false，
+    // 会把 400ms 的淡出直接砍掉。
+    layouts.child_layouts.emplace_back(edge_light_view_.get(),
+                                       edge_light_view_->IsActive(),
+                                       non_devtools_contents_bounds);
+  }
+
   // The content scrim view should cover the entire contents bounds.
   CHECK(contents_scrim_view_);
   layouts.child_layouts.emplace_back(contents_scrim_view_.get(),
