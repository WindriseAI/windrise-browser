diff --git a/chrome/browser/ui/views/frame/contents_container_view.cc b/chrome/browser/ui/views/frame/contents_container_view.cc
index 413b2f626d..f2016e127b 100644
--- a/chrome/browser/ui/views/frame/contents_container_view.cc
+++ b/chrome/browser/ui/views/frame/contents_container_view.cc
@@ -4,12 +4,17 @@
 
 #include "chrome/browser/ui/views/frame/contents_container_view.h"
 
+#include <algorithm>
 #include <memory>
 #include <optional>
 #include <utility>
 
+#include "base/functional/bind.h"
 #include "base/i18n/rtl.h"
 #include "chrome/browser/actor/ui/actor_overlay_web_view.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
 #include "chrome/browser/enterprise/data_protection/data_protection_overlay_view.h"
 #include "chrome/browser/glic/browser_ui/context_sharing_border_view.h"
@@ -20,11 +25,20 @@
 #include "chrome/browser/ui/read_anything/read_anything_immersive_overlay_view.h"
 #include "chrome/browser/ui/ui_features.h"
 #include "chrome/browser/ui/view_ids.h"
+#include "chrome/browser/extensions/extension_tab_util.h"
+#include "chrome/browser/extensions/windrise_extension_protection.h"
+#include "extensions/browser/extension_registry.h"
 #include "chrome/browser/ui/views/frame/browser_view.h"
 #include "chrome/browser/ui/views/frame/contents_capture_border_view.h"
 #include "chrome/browser/ui/views/frame/contents_container_outline.h"
 #include "chrome/browser/ui/views/frame/contents_separator.h"
 #include "chrome/browser/ui/views/frame/contents_web_view.h"
+#include "chrome/browser/ui/views/frame/windrise_edge_light_view.h"
+#include "chrome/browser/ui/views/frame/windrise_edge_light_web_view.h"
+#include "base/strings/string_number_conversions.h"
+#include "chrome/browser/profiles/profile.h"
+#include "components/prefs/pref_service.h"
+#include "url/gurl.h"
 #include "chrome/browser/ui/views/frame/multi_contents_view_mini_toolbar.h"
 #include "chrome/browser/ui/views/frame/scrim_view.h"
 #include "chrome/browser/ui/views/frame/tab_modal_dialog_host.h"
@@ -156,6 +170,39 @@ ContentsContainerView::ContentsContainerView(BrowserView* browser_view)
             .Build());
   }
 
+  // 风桥 N11：边缘光效层，画在网页区上方（层 = 网页区，外扩 0），不接输入。
+  // N10：网页角 12 -> 36 的过渡进度由它按合成器时间戳算，经回调交给本类。
+  edge_light_view_ = AddChildView(std::make_unique<WindriseEdgeLightView>());
+  edge_light_view_->SetCanProcessEventsWithinSubtree(false);
+  edge_light_view_->SetCornerProgressCallback(base::BindRepeating(
+      &ContentsContainerView::OnAiCornerProgress, base::Unretained(this)));
+  // 风桥 N11 独立层：和光效层同一个位置、在它上面；平时不建网页视图（第一次显示才建）。
+  edge_light_web_view_ = AddChildView(
+      std::make_unique<WindriseEdgeLightWebView>(browser_view->browser()));
+  // 光效画在哪由偏好定，扩展经 browserOS.setEdgeLightMode 写；改了立刻生效。
+  edge_light_prefs_.Init(browser_view->GetProfile()->GetPrefs());
+  edge_light_prefs_.Add(
+      browseros::prefs::kWindriseEdgeLightMode,
+      base::BindRepeating(&ContentsContainerView::ApplyEdgeLightMode,
+                          base::Unretained(this)));
+  edge_light_prefs_.Add(
+      browseros::prefs::kWindriseEdgeLightExtensionId,
+      base::BindRepeating(&ContentsContainerView::ApplyEdgeLightMode,
+                          base::Unretained(this)));
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
+  assistant_authority_subscription_ =
+      extensions::AddWindriseAssistantAuthorityObserver(base::BindRepeating(
+          &ContentsContainerView::SyncEdgeLightWebLayer, base::Unretained(this)));
+
   mini_toolbar_ = AddChildView(std::make_unique<MultiContentsViewMiniToolbar>(
       browser_view, contents_view_));
 
@@ -166,9 +213,21 @@ ContentsContainerView::ContentsContainerView(BrowserView* browser_view)
       AddChildView(std::make_unique<ContentsCaptureBorderView>(mini_toolbar_));
 
   view_bounds_observer_.Observe(contents_view_);
+  ApplyEdgeLightMode();
 }
 
 ContentsContainerView::~ContentsContainerView() {
+  ai_state_subscription_ = {};
+  ai_pulse_subscription_ = {};
+  assistant_authority_subscription_ = {};
+  edge_light_prefs_.RemoveAll();
+  edge_light_web_view_ = nullptr;
+  if (edge_light_view_) {
+    // 子视图在基类析构里才删，回调不能再指向已析构的本类。
+    edge_light_view_->SetCornerProgressCallback({});
+  }
+  browseros::windrise::AiWindowState::Detach(windrise_ai_window_);
+  edge_light_view_ = nullptr;
   indigo_overlay_view_ = nullptr;
   // read_anything_immersive_overlay_view_ holds a raw_ptr to
   // contents_view_. We need to make sure we destroy
@@ -304,10 +363,62 @@ void ContentsContainerView::SetBorderRoundedCornersFrom(
   if (glic_border_) {
     glic_border_->SetRoundedCorners(content_rounded_corners);
   }
+
+  if (edge_light_view_) {
+    // 光的外沿贴着网页角：同一组数（含 N10 的 AI 过渡）。
+    edge_light_view_->SetCornerRadii(content_rounded_corners);
+  }
+  if (edge_light_web_view_) {
+    // 独立层的圆角同上（和 ActorOverlayWebView 一样用 holder 的原生圆角）。
+    edge_light_web_view_->SetCornerRadii(content_rounded_corners);
+    // 光本身由扩展页画：全屏、devtools 停靠、分屏切换时它该用的角跟着变，
+    // 换一下独立层地址的查询串（只有变了才重新加载，AI 过渡的每一帧不会触发）。
+    if (edge_light_web_view_->shown()) {
+      const GURL url = EdgeLightWebLayerUrl();
+      content::WebContents* layer = edge_light_web_view_->web_contents();
+      if (url.is_valid() && layer && layer->GetVisibleURL() != url) {
+        edge_light_web_view_->SetShown(true, url);
+      }
+    }
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
+  // 光效层每帧回调一次（淡出结束回到 0 时一定回调）：独立层的显示 / 隐藏跟着它。
+  SyncEdgeLightWebLayer();
+  if (GetEffectiveRoundedCorners() == last_effective_radii_) {
+    return;
+  }
+  // 逐帧应用：devtools 停靠侧 / 新标签页 footer 的角由 SetBorderRoundedCornersFrom
+  // 逐角置 0 的逻辑原样生效。
+  UpdateBorderRoundedCorners();
 }
 
 void ContentsContainerView::ChildVisibilityChanged(View* child) {
@@ -318,6 +429,9 @@ void ContentsContainerView::ChildVisibilityChanged(View* child) {
 }
 
 void ContentsContainerView::Layout(PassKey pass_key) {
+  // 切标签时网页区的 bounds 不一定变，但显示的是另一个标签；每次布局顺带按
+  // 当前 WebContents 的 AI 状态同步一次开 / 关（幂等，不改布局）。
+  SyncEdgeLight();
   LayoutSuperclass<views::View>(this);
 
   UpdateContentsClip();
@@ -341,7 +455,122 @@ void ContentsContainerView::OnViewBoundsChanged(View* observed_view) {
     if (!rounded_corner_radii_.IsEmpty()) {
       UpdateBorderRoundedCorners();
     }
+    UpdateEdgeLight();
+  }
+}
+
+void ContentsContainerView::UpdateEdgeLight() {
+  if (!edge_light_view_) {
+    return;
   }
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
+  SyncEdgeLightWebLayer();
+}
+
+void ContentsContainerView::ApplyEdgeLightMode() {
+  const std::string mode = browser_view_->GetProfile()->GetPrefs()->GetString(
+      browseros::prefs::kWindriseEdgeLightMode);
+  // 只有 page / layer 两种（原生 Skia 画法已摘除，别的值都按 page）。
+  // 两种模式下 edge_light_view_ 都照常计时（N10 网页角 12 -> 36、IsActive），它本身不画。
+  edge_light_mode_ = mode == "layer" ? "layer" : "page";
+  UpdateEdgeLight();
+}
+
+GURL ContentsContainerView::EdgeLightWebLayerUrl() const {
+  const std::string id = browser_view_->GetProfile()->GetPrefs()->GetString(
+      browseros::prefs::kWindriseEdgeLightExtensionId);
+  // 扩展 id：32 个 a-p 小写字母（和 crx_file::id_util::IdIsValid 同一规则）。
+  if (id.size() != 32 ||
+      !std::all_of(id.begin(), id.end(),
+                   [](char c) { return c >= 'a' && c <= 'p'; })) {
+    return GURL();
+  }
+  // 光由扩展页自己画、原生只裁剪，所以把光走线该用的四角带给它（放查询串，
+  // #native&w= 不变，旧扩展照旧认得）：r0 = 平时的角，r1 = AI 操作时要到的角。
+  // devtools 停靠侧、新标签页 footer 下边的角两者都是 0（同 SetBorderRoundedCornersFrom
+  // 对网页区的裁剪）；全屏（F11、HTML 元素全屏）、分屏不做 36，r1 = r0
+  // （编译当天清单 §7 第 4 项）。
+  const bool devtools = devtools_web_view_ && devtools_web_view_->GetVisible();
+  const bool footer =
+      new_tab_footer_view_ && new_tab_footer_view_->GetVisible();
+  const DevToolsDockedPlacement dock = current_devtools_docked_placement_;
+  const bool flat_ul = devtools && dock == DevToolsDockedPlacement::kLeft;
+  const bool flat_ur = devtools && dock == DevToolsDockedPlacement::kRight;
+  const bool flat_lr =
+      footer || (devtools && (dock == DevToolsDockedPlacement::kBottom ||
+                              dock == DevToolsDockedPlacement::kRight));
+  const bool flat_ll =
+      footer || (devtools && (dock == DevToolsDockedPlacement::kBottom ||
+                              dock == DevToolsDockedPlacement::kLeft));
+  const auto corners = [&](float ul, float ur, float lr, float ll) {
+    const auto px = [](bool flat, float v) {
+      return base::NumberToString(flat ? 0 : static_cast<int>(v + 0.5f));
+    };
+    return px(flat_ul, ul) + "," + px(flat_ur, ur) + "," + px(flat_lr, lr) +
+           "," + px(flat_ll, ll);
+  };
+  const gfx::RoundedCornersF& base_radii = rounded_corner_radii_;
+  const std::string r0 =
+      corners(base_radii.upper_left(), base_radii.upper_right(),
+              base_radii.lower_right(), base_radii.lower_left());
+  content::WebContents* web_contents = contents_view_->web_contents();
+  const bool ai_corners = !base_radii.IsEmpty() && !is_in_split_ &&
+                          !browser_view_->IsFullscreen() &&
+                          !(web_contents && web_contents->IsFullscreen());
+  const float ai = static_cast<float>(
+      browseros::windrise::GetLayoutMetrics().Get(
+          browseros::windrise::LayoutMetric::kWebContentsRadiusAi));
+  const std::string r1 = ai_corners ? corners(ai, ai, ai, ai) : r0;
+  // 地址只由扩展 id 拼：路径固定，调用方改不了它指向哪。
+  return GURL("chrome-extension://" + id + "/edge-light/frame.html?r0=" + r0 +
+              "&r1=" + r1 + "#native&w=" +
+              base::NumberToString(extensions::ExtensionTabUtil::GetWindowId(
+                  browser_view_->browser())));
+}
+
+void ContentsContainerView::SyncEdgeLightWebLayer() {
+  if (!edge_light_web_view_ || !edge_light_view_) {
+    return;
+  }
+  Profile* profile = browser_view_->GetProfile();
+  const std::string& id = profile->GetPrefs()->GetString(
+      browseros::prefs::kWindriseEdgeLightExtensionId);
+  auto* registry = extensions::ExtensionRegistry::Get(profile);
+  const auto* extension = registry->enabled_extensions().GetByID(id);
+  if (!extensions::IsAuthorizedWindriseExtension(extension)) {
+    edge_light_web_view_->Revoke();
+    return;
+  }
+  const bool want = edge_light_mode_ == "layer" && edge_light_view_->IsActive();
+  if (want == edge_light_web_view_->shown()) {
+    return;
+  }
+  edge_light_web_view_->SetShown(want, want ? EdgeLightWebLayerUrl() : GURL());
+  InvalidateLayout();
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
 }
 
 void ContentsContainerView::SetContentsResizingStrategy(
@@ -537,6 +766,19 @@ views::ProposedLayout ContentsContainerView::CalculateProposedLayout(
                                        non_devtools_contents_bounds);
   }
 
+  if (edge_light_view_) {
+    // 风桥 N10 / N11 计时器：只计时、不绘制，一直不可见（独立层另算，见下）。
+    layouts.child_layouts.emplace_back(edge_light_view_.get(),
+                                       /*visible=*/false,
+                                       non_devtools_contents_bounds);
+  }
+  if (edge_light_web_view_) {
+    // 风桥 N11 独立层：同一个位置（网页区，外扩 0），在光效层上面。
+    layouts.child_layouts.emplace_back(edge_light_web_view_.get(),
+                                       edge_light_web_view_->shown(),
+                                       non_devtools_contents_bounds);
+  }
+
   // The content scrim view should cover the entire contents bounds.
   CHECK(contents_scrim_view_);
   layouts.child_layouts.emplace_back(contents_scrim_view_.get(),
