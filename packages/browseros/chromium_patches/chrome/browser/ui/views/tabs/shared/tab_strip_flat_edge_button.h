diff --git a/chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.h b/chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.h
index c227115..8f5a063 100644
--- a/chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.h
+++ b/chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.h
@@ -5,7 +5,10 @@
 #ifndef CHROME_BROWSER_UI_VIEWS_TABS_SHARED_TAB_STRIP_FLAT_EDGE_BUTTON_H_
 #define CHROME_BROWSER_UI_VIEWS_TABS_SHARED_TAB_STRIP_FLAT_EDGE_BUTTON_H_
 
+#include <optional>
+
 #include "ui/base/metadata/metadata_impl_macros.h"
+#include "ui/color/color_id.h"
 #include "ui/views/controls/button/label_button.h"
 #include "ui/views/layout/layout_types.h"
 #include "ui/views/masked_targeter_delegate.h"
@@ -38,6 +41,10 @@ class TabStripFlatEdgeButton : public views::LabelButton,
 
   void SetFlatEdge(FlatEdge flat_edge);
   void SetCornerRadius(float corner_radius);
+  // 风桥：覆盖按钮底 / 图标颜色。默认跟随上游 tab strip 控件色；竖向标签栏的
+  // 顶部小按钮与整宽「+」用 kColorWindriseTileBackground / kColorTabForeground*。
+  void SetBackgroundColorId(std::optional<ui::ColorId> color_id);
+  void SetForegroundColorId(std::optional<ui::ColorId> color_id);
   void SetIconSize(int icon_size);
   void UpdateIcon(const ui::ImageModel& icon_image);
   void SetInsets(const gfx::Insets& insets);
@@ -94,6 +101,8 @@ class TabStripFlatEdgeButton : public views::LabelButton,
   float flat_edge_factor_ = 1.0f;
   FlatEdge flat_edge_ = FlatEdge::kNone;
   std::optional<float> corner_radius_;
+  std::optional<ui::ColorId> background_color_id_;
+  std::optional<ui::ColorId> foreground_color_id_;
   bool paint_transparent_ = false;
   base::CallbackListSubscription paint_as_active_subscription_;
 
