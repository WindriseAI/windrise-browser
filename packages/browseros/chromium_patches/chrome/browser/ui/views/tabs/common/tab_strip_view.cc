diff --git a/chrome/browser/ui/views/tabs/common/tab_strip_view.cc b/chrome/browser/ui/views/tabs/common/tab_strip_view.cc
index 50da31fe97..183ffaeb0c 100644
--- a/chrome/browser/ui/views/tabs/common/tab_strip_view.cc
+++ b/chrome/browser/ui/views/tabs/common/tab_strip_view.cc
@@ -9,6 +9,7 @@
 #include "base/memory/raw_ptr.h"
 #include "base/metrics/histogram_macros.h"
 #include "base/scoped_multi_source_observation.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
 #include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/ui/browser_element_identifiers.h"
 #include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
@@ -45,6 +46,23 @@
 #include "ui/views/view_class_properties.h"
 #include "ui/views/view_utils.h"
 
+namespace {
+
+// 风桥 N6：竖向标签条上段不画 AI 的标签组（见 UnpinnedTabContainerView），
+// 这些组里的标签由侧边栏下段画。AI 切到自己的标签时，上段不该为一个看不见的
+// 标签滚动。
+bool IsInWindriseAiGroup(const views::View* view) {
+  for (const views::View* v = view; v; v = v->parent()) {
+    if (const auto* group_view = views::AsViewClass<TabGroupView>(v)) {
+      return browseros::windrise::AiGroups::Contains(
+          group_view->GetTabGroup().id());
+    }
+  }
+  return false;
+}
+
+}  // namespace
+
 class TabStripView::TargetViewsTracker : public views::ViewObserver {
  public:
   explicit TargetViewsTracker(TabStripOrientation orientation)
@@ -377,6 +395,14 @@ void TabStripView::ScrollToFitTabs(const tabs::TabInterface* active_tab,
 }
 
 void TabStripView::ScrollToFitViews(views::View* view1, views::View* view2) {
+  if (orientation_ == TabStripOrientation::kVertical) {
+    if (IsInWindriseAiGroup(view1)) {
+      view1 = nullptr;
+    }
+    if (IsInWindriseAiGroup(view2)) {
+      view2 = nullptr;
+    }
+  }
   views::View* primary_view = (view1 && Contains(view1)) ? view1 : nullptr;
   views::View* secondary_view = (view2 && Contains(view2)) ? view2 : nullptr;
   if (!primary_view && !secondary_view) {
