diff --git a/chrome/browser/ui/views/frame/contents_container_view.h b/chrome/browser/ui/views/frame/contents_container_view.h
index e3ae9d1..ff69621 100644
--- a/chrome/browser/ui/views/frame/contents_container_view.h
+++ b/chrome/browser/ui/views/frame/contents_container_view.h
@@ -8,6 +8,7 @@
 #include <memory>
 #include <optional>
 
+#include "base/callback_list.h"
 #include "base/memory/raw_ptr.h"
 #include "base/scoped_observation.h"
 #include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
@@ -20,6 +21,7 @@
 class BrowserView;
 class ContentsCaptureBorderView;
 class ContentsContainerOutline;
+class WindriseSilverWaveView;
 class ContentsWebView;
 class MultiContentsViewMiniToolbar;
 class ScrimView;
@@ -154,6 +156,11 @@ class ContentsContainerView : public views::View,
   void UpdateBorderRoundedCorners();
   void SetBorderRoundedCornersFrom(const gfx::RoundedCornersF& corner_radii);
 
+  // 风桥 N11：按 AI 状态（browserOS.setAiActivity）开 / 关网页四周的银浪。
+  void UpdateSilverWave();
+  // 只同步开 / 关，不动布局（Layout 里调，避免 InvalidateLayout 递归）。
+  void SyncSilverWaveVisibility();
+
   // views::View:
   void ChildVisibilityChanged(View* child) override;
   void Layout(PassKey) override;
@@ -169,6 +176,9 @@ class ContentsContainerView : public views::View,
   bool is_in_split_ = false;
 
   raw_ptr<BrowserView> browser_view_ = nullptr;
+  // 风桥 N11：AI 操作时的银色海浪层（挂在网页区上方，不接输入）。
+  raw_ptr<WindriseSilverWaveView> silver_wave_view_ = nullptr;
+  base::CallbackListSubscription ai_state_subscription_;
 
   // An invisible view used to anchor tab toasts to the top of the contents
   // view, while being before the contents view in the focus order.
