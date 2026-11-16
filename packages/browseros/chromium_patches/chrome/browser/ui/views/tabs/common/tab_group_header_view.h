diff --git a/chrome/browser/ui/views/tabs/common/tab_group_header_view.h b/chrome/browser/ui/views/tabs/common/tab_group_header_view.h
index 1645b25..88883b9 100644
--- a/chrome/browser/ui/views/tabs/common/tab_group_header_view.h
+++ b/chrome/browser/ui/views/tabs/common/tab_group_header_view.h
@@ -19,6 +19,7 @@
 #include "components/tab_groups/tab_group_id.h"
 #include "components/tab_groups/tab_group_visual_data.h"
 #include "ui/base/metadata/metadata_header_macros.h"
+#include "ui/gfx/font_list.h"
 #include "ui/gfx/geometry/rounded_corners_f.h"
 #include "ui/views/context_menu_controller.h"
 #include "ui/views/focus/focus_manager.h"
@@ -151,6 +152,9 @@ class TabGroupHeaderView : public views::FlexLayoutView,
   SkColor GetBackgroundColor() const;
   SkColor GetForegroundColor() const;
   void UpdateColors();
+  // 风桥 AI 那一段的标题按段标题的字体 / 行高排（偏好
+  // browseros.windrise.layout.ai_section_header_*）。
+  void UpdateAiSectionStyle();
 
   const raw_ref<Delegate> delegate_;
   const TabStripOrientation orientation_;
@@ -162,6 +166,9 @@ class TabGroupHeaderView : public views::FlexLayoutView,
   // the tabstrip.
   const raw_ptr<views::ImageView> sync_icon_ = nullptr;
   const raw_ptr<views::Label> group_header_label_ = nullptr;
+  // 上游给组标题的字体，AI 段标题改字号后用它还原。
+  gfx::FontList default_header_font_;
+  bool header_font_captured_ = false;
   // The circle indicator rendered after the title when a tab group needs
   // attention.
   const raw_ptr<views::ImageView> attention_indicator_ = nullptr;
