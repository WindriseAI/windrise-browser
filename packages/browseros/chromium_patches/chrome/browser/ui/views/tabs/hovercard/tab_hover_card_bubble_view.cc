diff --git a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
index e582c70239..506a23a057 100644
--- a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
+++ b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.cc
@@ -15,6 +15,7 @@
 #include "base/metrics/field_trial_params.h"
 #include "build/build_config.h"
 #include "chrome/app/vector_icons/vector_icons.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/color/chrome_color_id.h"
 #include "chrome/browser/ui/performance_controls/tab_resource_usage_tab_helper.h"
 #include "chrome/browser/ui/tabs/saved_tab_groups/collaboration_messaging_tab_data.h"
@@ -380,9 +381,18 @@ class TabHoverCardBubbleView::TabCardView : public views::View {
     CHECK(bubble_view_);
     SetProperty(views::kElementIdentifierKey, kTabCardElementId);
 
-    title_label_ = AddChildView(std::make_unique<FadeLabelView>(
-        kHoverCardTitleMaxLines, CONTEXT_TAB_HOVER_CARD_TITLE,
-        views::style::STYLE_BODY_3_EMPHASIS));
+    // 风桥 theme-v2 §4：收起侧栏上悬停显示完整标题，提示框文字 12（上游标题
+    // 13–14）。用 12 号中等字重，和下面 12 号常规的域名仍分得出主次。展开态
+    // 保持上游原样。
+    title_label_ =
+        bubble_view_->bubble_params().windrise_rail_style
+            ? AddChildView(std::make_unique<FadeLabelView>(
+                  kHoverCardTitleMaxLines,
+                  views::style::CONTEXT_DIALOG_BODY_TEXT,
+                  views::style::STYLE_BODY_4_MEDIUM))
+            : AddChildView(std::make_unique<FadeLabelView>(
+                  kHoverCardTitleMaxLines, CONTEXT_TAB_HOVER_CARD_TITLE,
+                  views::style::STYLE_BODY_3_EMPHASIS));
 
     domain_label_ = AddChildView(std::make_unique<FadeLabelView>(
         1, views::style::CONTEXT_DIALOG_BODY_TEXT, views::style::STYLE_BODY_4));
@@ -665,6 +675,18 @@ TabHoverCardBubbleView::TabHoverCardBubbleView(
 #if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
   set_accept_events(false);
 #endif
+  // Windrise : the hover card runs its own WidgetFadeAnimator;
+  // keep upstream's default visibility animation on every platform,
+  // Windows included (decision d: unchanged).
+  set_windrise_use_default_visibility_animation(true);
+  // 风桥 theme-v2 §4：收起侧栏上的提示框圆角 8（偏好 radius_small，「输入框、
+  // 行、提示」）；展开态保持上游圆角。要在下面建 TabCardView 之前设：缩略图的
+  // 圆角也读它。底色不改：气泡底是 kColorBubbleBackground，风桥 mixer 给的是
+  // popup，默认值和 --wr-tab-on 相同（#F7F8F9 / #3C434C）。
+  if (params.windrise_rail_style) {
+    corner_radius_ = browseros::windrise::GetLayoutMetrics().Get(
+        browseros::windrise::LayoutMetric::kRadiusSmall);
+  }
   set_accept_drag_drop(false);
 
   // Set so that the tab hover card is not focus traversable when keyboard
