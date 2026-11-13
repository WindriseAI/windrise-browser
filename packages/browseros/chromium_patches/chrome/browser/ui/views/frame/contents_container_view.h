diff --git a/chrome/browser/ui/views/frame/contents_container_view.h b/chrome/browser/ui/views/frame/contents_container_view.h
index e3ae9d104d..bceff72ce6 100644
--- a/chrome/browser/ui/views/frame/contents_container_view.h
+++ b/chrome/browser/ui/views/frame/contents_container_view.h
@@ -7,22 +7,33 @@
 
 #include <memory>
 #include <optional>
+#include <string>
 
+#include "base/callback_list.h"
 #include "base/memory/raw_ptr.h"
 #include "base/scoped_observation.h"
 #include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
 #include "chrome/browser/ui/views/frame/tab_modal_dialog_host.h"
+#include "components/prefs/pref_change_registrar.h"
 #include "ui/gfx/geometry/rounded_corners_f.h"
 #include "ui/views/focus/external_focus_tracker.h"
 #include "ui/views/layout/delegating_layout_manager.h"
 #include "ui/views/view.h"
+#include "url/gurl.h"
 
 class BrowserView;
+class BrowserWindowInterface;
 class ContentsCaptureBorderView;
 class ContentsContainerOutline;
+class WindriseEdgeLightView;
+class WindriseEdgeLightWebView;
 class ContentsWebView;
 class MultiContentsViewMiniToolbar;
 class ScrimView;
+
+namespace browseros::windrise {
+struct AiPulse;
+}  // namespace browseros::windrise
 class ActorOverlayWebView;
 class ReadAnythingImmersiveOverlayView;
 
@@ -154,6 +165,27 @@ class ContentsContainerView : public views::View,
   void UpdateBorderRoundedCorners();
   void SetBorderRoundedCornersFrom(const gfx::RoundedCornersF& corner_radii);
 
+  // 风桥 N11：按 AI 状态（browserOS.setAiActivity）开 / 关网页四周的边缘光效。
+  void UpdateEdgeLight();
+  // 只同步开 / 关，不动布局（Layout 里调，避免 InvalidateLayout 递归）。
+  void SyncEdgeLight();
+  // AI 的一次动作（browserOS.pulseAiActivity）。
+  void OnAiPulse(const browseros::windrise::AiPulse& pulse);
+  // 光效画在哪（偏好 browseros.windrise.edge_light_mode）：page / layer。
+  void ApplyEdgeLightMode();
+  // 独立层（mode = layer）跟着光效层的可见性（含淡出）显示 / 隐藏。
+  void SyncEdgeLightWebLayer();
+  // 独立层加载的地址：chrome-extension://<扩展 id>/edge-light/frame.html#native&w=<窗口 id>；
+  // 扩展没登记时无效。
+  GURL EdgeLightWebLayerUrl() const;
+
+  // 风桥 N10：AI 操作时网页区四角 12 -> 36 的过渡。进度 0..1 由边缘光效视图按
+  // 合成器时间戳算出来（同一个动画值，光的外沿因此始终贴着网页角）。
+  void OnAiCornerProgress(float progress);
+  // 基准圆角（rounded_corner_radii_）与 AI 过渡逐角合成。全屏 / 网页全屏 /
+  // 分屏、基准为空（不圆角）时不做过渡。
+  gfx::RoundedCornersF GetEffectiveRoundedCorners() const;
+
   // views::View:
   void ChildVisibilityChanged(View* child) override;
   void Layout(PassKey) override;
@@ -169,6 +201,20 @@ class ContentsContainerView : public views::View,
   bool is_in_split_ = false;
 
   raw_ptr<BrowserView> browser_view_ = nullptr;
+  // 风桥 N11：AI 操作时的边缘光效层（挂在网页区上方，不接输入）。
+  raw_ptr<WindriseEdgeLightView> edge_light_view_ = nullptr;
+  base::CallbackListSubscription ai_state_subscription_;
+  base::CallbackListSubscription ai_pulse_subscription_;
+  base::CallbackListSubscription assistant_authority_subscription_;
+  // N10：AI 操作时网页角过渡进度（0 = 基准，1 = 36）与最近一次应用的有效圆角。
+  float ai_corner_progress_ = 0.0f;
+  gfx::RoundedCornersF last_effective_radii_;
+  // AiWindowState 的键（只比较指针，不解引用）。
+  raw_ptr<BrowserWindowInterface> windrise_ai_window_ = nullptr;
+  // 风桥 N11 独立层：透明、不接输入的网页视图，加载扩展的 edge-light/frame.html。
+  raw_ptr<WindriseEdgeLightWebView> edge_light_web_view_ = nullptr;
+  std::string edge_light_mode_;
+  PrefChangeRegistrar edge_light_prefs_;
 
   // An invisible view used to anchor tab toasts to the top of the contents
   // view, while being before the contents view in the focus order.
