diff --git a/chrome/browser/ui/views/tabs/common/tab_view.cc b/chrome/browser/ui/views/tabs/common/tab_view.cc
index 7ad4897a75..a5b975e9bb 100644
--- a/chrome/browser/ui/views/tabs/common/tab_view.cc
+++ b/chrome/browser/ui/views/tabs/common/tab_view.cc
@@ -6,12 +6,17 @@
 
 #include <optional>
 #include <string>
+#include <string_view>
 
 #include "base/functional/callback_helpers.h"
+#include "base/i18n/char_iterator.h"
 #include "base/metrics/histogram_functions.h"
 #include "base/metrics/user_metrics.h"
 #include "base/metrics/user_metrics_action.h"
+#include "base/strings/utf_string_conversion_utils.h"
+#include "base/strings/utf_string_conversions.h"
 #include "base/time/time.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/dictation/dictation_keyed_service.h"
 #include "chrome/browser/glic/browser_ui/tab_underline_controller.h"
 #include "chrome/browser/glic/browser_ui/tab_underline_view.h"
@@ -65,6 +70,7 @@
 #include "components/tabs/public/tab_collection_types.h"
 #include "components/tabs/public/tab_interface.h"
 #include "content/public/browser/navigation_controller.h"
+#include "third_party/icu/source/common/unicode/uchar.h"
 #include "third_party/skia/include/core/SkPath.h"
 #include "third_party/skia/include/core/SkPathBuilder.h"
 #include "third_party/skia/include/core/SkRRect.h"
@@ -101,6 +107,48 @@
 namespace {
 constexpr int kFocusRingInset = 0.0f;
 
+// 风桥 theme-v2 §4：折叠窄条上每个标签露出的那一个字。
+// 按码点走（不按 UTF-16 单元），增补平面的汉字、emoji 不会被切成半个代理对。
+// 取第一个「字母类」字符（Unicode 的 L* 类）：中文即第一个汉字，英文取首字母
+// 再转大写，假名、韩文、西里尔、希腊等同理（无大小写的文字原样）。前面的
+// emoji、标点、空格、「(3) 」这类未读数都跳过。一个字母都没有时取第一个数字。
+std::u16string FirstRailGlyphIn(std::u16string_view text) {
+  std::optional<base_icu::UChar32> first_digit;
+  for (base::i18n::UTF16CharIterator it(text); !it.end(); it.Advance()) {
+    const base_icu::UChar32 c = it.get();
+    if (u_isalpha(c)) {
+      std::u16string glyph;
+      base::WriteUnicodeCharacter(u_toupper(c), &glyph);
+      return glyph;
+    }
+    if (!first_digit && u_isdigit(c)) {
+      first_digit = c;
+    }
+  }
+  std::u16string glyph;
+  if (first_digit) {
+    base::WriteUnicodeCharacter(*first_digit, &glyph);
+  }
+  return glyph;
+}
+
+// 标题里取不到（空标题、正在加载还没有标题、全是 emoji / 符号）就取域名
+// （去掉开头的 www.）的第一个字母或数字；也取不到（about:blank 等）就空着，
+// 窄条上只显示图标，图标位置不变。
+std::u16string RailGlyphFor(const tabs::TabData& data) {
+  if (!data.should_render_loading_title) {
+    std::u16string glyph = FirstRailGlyphIn(data.title);
+    if (!glyph.empty()) {
+      return glyph;
+    }
+  }
+  std::string_view host = data.visible_url.host();
+  if (host.starts_with("www.")) {
+    host.remove_prefix(4);
+  }
+  return FirstRailGlyphIn(base::ASCIIToUTF16(host));
+}
+
 class TabHighlightPathGenerator : public views::HighlightPathGenerator {
  public:
   explicit TabHighlightPathGenerator(TabView* tab_view) : tab_view_(tab_view) {}
@@ -366,6 +414,22 @@ TabView::TabView(TabCollectionNode* collection_node)
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
@@ -1209,6 +1273,7 @@ void TabView::UpdateTabData(const tabs::TabData& data) {
   icon_->SetAttention(TabIcon::AttentionType::kTabWantsAttentionStatus,
                       tab_data_.needs_attention);
   UpdateTitle(tab_data_.title, tab_data_.should_render_loading_title);
+  UpdateRailGlyph();
   alert_indicator_->TransitionToAlertState(tab_data_.alert_state);
   SetHoverCardDataFrom(tab_data_);
 }
@@ -1229,6 +1294,35 @@ void TabView::UpdateTitle(std::u16string title,
   title_->SetText(title);
 }
 
+void TabView::UpdateRailGlyph() {
+  if (!rail_glyph_) {
+    return;
+  }
+  const std::u16string glyph = RailGlyphFor(tab_data_);
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
@@ -1260,6 +1354,11 @@ void TabView::UpdateColors() {
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
 
