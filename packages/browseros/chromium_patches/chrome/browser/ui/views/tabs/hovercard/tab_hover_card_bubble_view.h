diff --git a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.h b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.h
index 4091112541..35f208c18b 100644
--- a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.h
+++ b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_bubble_view.h
@@ -57,6 +57,9 @@ class TabHoverCardBubbleView : public views::BubbleDialogDelegateView {
     bool show_image_preview = true;
     // Becomes false for ChromeOS system apps or when disabled in settings
     bool show_memory_usage = true;
+    // 风桥 theme-v2 §4：悬停的是收起侧栏（70 宽）里的标签时为 true，卡片用
+    // 圆角 8、标题 12 号；展开态（含悬停展开）保持上游原样（协调人 1116 裁定）。
+    bool windrise_rail_style = false;
   };
 
   explicit TabHoverCardBubbleView(HoverCardAnchorTarget* anchor_target,
