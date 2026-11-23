diff --git a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.cc b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.cc
index 2358db3c42..1feee8ebe8 100644
--- a/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.cc
+++ b/chrome/browser/ui/views/tabs/hovercard/tab_hover_card_controller.cc
@@ -35,7 +35,9 @@
 #include "ui/events/event_observer.h"
 #include "ui/events/types/event_type.h"
 #include "ui/views/event_monitor.h"
+#include "ui/views/controls/label.h"
 #include "ui/views/view.h"
+#include "ui/views/view_utils.h"
 #include "ui/views/widget/widget.h"

 namespace {
@@ -70,6 +72,47 @@ base::TimeDelta GetPreviewImageCaptureDelay(
   return base::Milliseconds(ms);
 }

+// 风桥（Opus-2 11-23，根因分析 B7③）：竖向标签栏展开、标签标题完整显示时，
+// 悬停再弹一张带内存读数的大白底卡片是冗余的（标题就在眼前）。只在两种情况下
+// 还弹：窄轨收起态（只剩图标 + 一个字），或者标题被截断（省略 / 渐隐 / 被裁）。
+// 找标题的办法不依赖标签视图的具体类：在锚点视图的直接子视图和孙视图里找
+// 「可见、有字」的 views::Label，取最宽的那个（收起态的「首字」标签展开时是
+// 隐藏的，所以取到的就是标题）；一个都没有（固定标签的小方块）就照旧弹。
+const views::Label* FindVisibleTitleLabel(const views::View* view, int depth) {
+  const views::Label* best = nullptr;
+  for (const views::View* child : view->children()) {
+    if (!child->GetVisible()) {
+      continue;
+    }
+    const views::Label* candidate = views::AsViewClass<views::Label>(child);
+    if (!candidate && depth > 0) {
+      candidate = FindVisibleTitleLabel(child, depth - 1);
+    }
+    if (candidate && !candidate->GetText().empty() &&
+        (!best || candidate->width() > best->width())) {
+      best = candidate;
+    }
+  }
+  return best;
+}
+
+bool WindriseTitleFullyVisible(BrowserWindowInterface* browser,
+                               HoverCardAnchorTarget* anchor_target) {
+  const auto* vertical_state_controller =
+      tabs::VerticalTabStripStateController::From(browser);
+  if (!vertical_state_controller ||
+      !vertical_state_controller->ShouldDisplayVerticalTabs()) {
+    return false;  // 横向标签条：照上游。
+  }
+  const views::View* anchor_view = anchor_target->GetView();
+  if (!anchor_view ||
+      anchor_view->width() <= tabs::kVerticalTabStripCollapsedWidth) {
+    return false;  // 窄轨（含收起态的标签）：照旧弹。
+  }
+  const views::Label* title = FindVisibleTitleLabel(anchor_view, /*depth=*/1);
+  return title && !title->IsDisplayTextTruncated();
+}
+
 base::TimeDelta GetShowDelay(BrowserWindowInterface* browser,
                              TabStripRegionView* tab_strip,
                              HoverCardAnchorTarget* anchor_target) {
@@ -310,6 +353,15 @@ void TabHoverCardController::UpdateHoverCard(
     anchor_target = nullptr;
   }

+  // 风桥：竖向标签栏里标题完整可见的标签不弹悬停卡（见 WindriseTitleFullyVisible）。
+  // kTabDataChanged 必须带着正在显示的那张卡的锚点（下面的 CHECK），不在这时拦。
+  if (anchor_target &&
+      update_type != TabSlotController::HoverCardUpdateType::kTabDataChanged &&
+      WindriseTitleFullyVisible(browser_window_interface_.get(),
+                                anchor_target)) {
+    anchor_target = nullptr;
+  }
+
   // Update this ASAP so that if we try to fade-in and we have the wrong target
   // then when the fade timer elapses we won't incorrectly try to fade in on the
   // wrong tab.
@@ -444,6 +496,21 @@ void TabHoverCardController::CreateHoverCard(
   params.show_domain = !IsBrowserForSystemWebApp(browser_window_interface_);
   params.show_memory_usage = hover_card_memory_usage_enabled_;
   params.show_image_preview = hover_card_image_previews_enabled_;
+  // 风桥 theme-v2 §4：只有悬停收起侧栏（70 宽）里的标签时用收起态样式。侧栏
+  // 要是收起的（展开时固定标签也是小方块，不能只看宽），再看锚点自己的宽：
+  // 悬停展开（侧栏临时撑宽、显示标题）时锚点变宽，仍是上游样式。
+  if (const auto* vertical_state_controller =
+          tabs::VerticalTabStripStateController::From(
+              browser_window_interface_.get());
+      vertical_state_controller &&
+      vertical_state_controller->ShouldDisplayVerticalTabs() &&
+      vertical_state_controller->GetCollapseState() !=
+          tabs::VerticalTabStripCollapseState::kExpanded &&
+      anchor_target->GetView() &&
+      anchor_target->GetView()->width() <=
+          tabs::kVerticalTabStripCollapsedWidth) {
+    params.windrise_rail_style = true;
+  }

   hover_card_ = new TabHoverCardBubbleView(anchor_target, params);
   hover_card_observation_.Observe(hover_card_.get());
