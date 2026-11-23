diff --git a/chrome/browser/ui/views/tabs/common/tab_view.cc b/chrome/browser/ui/views/tabs/common/tab_view.cc
index 7ad4897a75..36ffccc804 100644
--- a/chrome/browser/ui/views/tabs/common/tab_view.cc
+++ b/chrome/browser/ui/views/tabs/common/tab_view.cc
@@ -6,12 +6,15 @@
 
 #include <optional>
 #include <string>
+#include <string_view>
 
 #include "base/functional/callback_helpers.h"
 #include "base/metrics/histogram_functions.h"
 #include "base/metrics/user_metrics.h"
 #include "base/metrics/user_metrics_action.h"
 #include "base/time/time.h"
+#include "chrome/browser/browseros/core/windrise_rail_glyph.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/dictation/dictation_keyed_service.h"
 #include "chrome/browser/glic/browser_ui/tab_underline_controller.h"
 #include "chrome/browser/glic/browser_ui/tab_underline_view.h"
@@ -101,6 +104,17 @@
 namespace {
 constexpr int kFocusRingInset = 0.0f;
 
+// 风桥 theme-v2 §4：折叠窄条上每个标签露出的那一个字。取字规则（汉字 → 英文
+// 字母大写 → 标题第一个字符 → 域名）在 browseros/core/windrise_rail_glyph.h，
+// 和侧栏下段的 AI 组合页共用。正在加载、还没有标题时（标题栏显示「加载中」，
+// 不取它）直接按域名取。
+std::u16string RailGlyphFor(const tabs::TabData& data) {
+  return browseros::windrise::RailGlyphFor(
+      data.should_render_loading_title ? std::u16string_view()
+                                       : std::u16string_view(data.title),
+      data.visible_url.host());
+}
+
 class TabHighlightPathGenerator : public views::HighlightPathGenerator {
  public:
   explicit TabHighlightPathGenerator(TabView* tab_view) : tab_view_(tab_view) {}
@@ -366,6 +380,22 @@ TabView::TabView(TabCollectionNode* collection_node)
   }
 
   title_->SetProperty(views::kElementIdentifierKey, kVerticalTabTitleElementId);
+  if (orientation_ == TabStripOrientation::kVertical) {
+    // 风桥 theme-v2 §4：折叠时图标右边那一个字。照 TabTitle 的设置：透明底、
+    // 不收事件、不出自己的提示（悬停照旧是悬停卡）；读屏用标签自己的名字，
+    // 这个字不单独读。默认隐藏，由竖向布局在折叠居中时打开。
+    rail_glyph_ = AddChildView(std::make_unique<views::Label>());
+    rail_glyph_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
+    rail_glyph_->SetElideBehavior(gfx::NO_ELIDE);
+    rail_glyph_->SetAutoColorReadabilityEnabled(false);
+    rail_glyph_->SetBackgroundColor(SK_ColorTRANSPARENT);
+    rail_glyph_->SetSkipSubpixelRenderingOpacityCheck(true);
+    rail_glyph_->SetHandlesTooltips(false);
+    rail_glyph_->SetCanProcessEventsWithinSubtree(false);
+    rail_glyph_->GetViewAccessibility().SetIsIgnored(true);
+    rail_glyph_->SetVisible(false);
+    UpdateRailGlyphFont();
+  }
   SetProperty(views::kElementIdentifierKey, kTabElementId);
   // Layout manager must be set after child views are created because the
   // vertical layout stores pointers to those children.
@@ -1209,6 +1239,7 @@ void TabView::UpdateTabData(const tabs::TabData& data) {
   icon_->SetAttention(TabIcon::AttentionType::kTabWantsAttentionStatus,
                       tab_data_.needs_attention);
   UpdateTitle(tab_data_.title, tab_data_.should_render_loading_title);
+  UpdateRailGlyph();
   alert_indicator_->TransitionToAlertState(tab_data_.alert_state);
   SetHoverCardDataFrom(tab_data_);
 }
@@ -1229,6 +1260,43 @@ void TabView::UpdateTitle(std::u16string title,
   title_->SetText(title);
 }
 
+void TabView::UpdateRailGlyph() {
+  if (!rail_glyph_) {
+    return;
+  }
+  std::u16string glyph = RailGlyphFor(tab_data_);
+  if (glyph.empty() &&
+      (!tab_data_.should_render_loading_title ||
+       !icon_->GetShowingLoadingAnimation())) {
+    // about:blank / data: / 无标题本地页既无标题也无域名：沿用 UpdateTitle 的
+    // 默认标题（如「无标题」/ "Untitled"）取首字，避免折起态漏渲染。
+    glyph = browseros::windrise::RailGlyphForTitle(
+        CoreTabHelper::GetDefaultTitle());
+  }
+  if (rail_glyph_->GetText() != glyph) {
+    rail_glyph_->SetText(glyph);
+    InvalidateLayout();
+  }
+}
+
+void TabView::UpdateRailGlyphFont() {
+  if (!rail_glyph_) {
+    return;
+  }
+  // 字号走偏好 rail_glyph_size（默认 14，theme-v2 §4），字重、字体族跟标题。
+  const int size = browseros::windrise::GetLayoutMetrics().Get(
+      browseros::windrise::LayoutMetric::kRailGlyphSize);
+  const gfx::FontList& title_font = title_->font_list();
+  if (rail_glyph_->font_list().GetFontSize() == size &&
+      rail_glyph_->font_list().GetFontWeight() ==
+          title_font.GetFontWeight()) {
+    return;
+  }
+  rail_glyph_->SetFontList(
+      title_font.DeriveWithSizeDelta(size - title_font.GetFontSize()));
+  InvalidateLayout();
+}
+
 void TabView::UpdateBorder() {
   if (orientation_ == TabStripOrientation::kHorizontal) {
     SetBorder(views::CreateEmptyBorder(tab_styling()->GetContentsInsets()));
@@ -1260,6 +1328,11 @@ void TabView::UpdateColors() {
   }
   TabStyle::TabColors colors = tab_styling()->CalculateTargetColors();
   title_->SetEnabledColor(colors.foreground_color);
+  if (rail_glyph_) {
+    // 那个字和标题同色：当前标签 --wr-text，其余 --wr-text-2。
+    rail_glyph_->SetEnabledColor(colors.foreground_color);
+    UpdateRailGlyphFont();
+  }
   close_button_->SetColors(colors);
   alert_indicator_->OnParentTabButtonColorChanged();
 
