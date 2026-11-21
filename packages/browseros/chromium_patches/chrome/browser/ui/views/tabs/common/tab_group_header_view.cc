diff --git a/chrome/browser/ui/views/tabs/common/tab_group_header_view.cc b/chrome/browser/ui/views/tabs/common/tab_group_header_view.cc
index 9e6a5d1..29fcc92 100644
--- a/chrome/browser/ui/views/tabs/common/tab_group_header_view.cc
+++ b/chrome/browser/ui/views/tabs/common/tab_group_header_view.cc
@@ -9,6 +9,8 @@
 #include "build/build_config.h"
 #include "build/buildflag.h"
 #include "chrome/app/vector_icons/vector_icons.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/ui/browser_element_identifiers.h"
 #include "chrome/browser/ui/color/chrome_color_id.h"
 #include "chrome/browser/ui/tabs/tab_group_data.h"
@@ -564,6 +566,7 @@ void TabGroupHeaderView::OnDataChanged(
 
   SetInteriorMargin(GetHeaderInsets());
   UpdateColors();
+  UpdateAiSectionStyle();
   if (GetColorProvider()) {
     UpdateAttentionState(
         data_sharing::features::IsDataSharingFunctionalityEnabled() &&
@@ -615,10 +618,47 @@ void TabGroupHeaderView::UpdateAccessibleName() {
       tab_groups::GetHoverCardAccessibilityText(delegate_->GetTabGroupData()));
 }
 
+void TabGroupHeaderView::UpdateAiSectionStyle() {
+  if (!group_header_label_) {
+    return;
+  }
+  if (!header_font_captured_) {
+    default_header_font_ = group_header_label_->font_list();
+    header_font_captured_ = true;
+  }
+  const bool is_ai_group =
+      delegate_->IsValid() &&
+      browseros::windrise::AiGroups::Contains(delegate_->GetTabGroup().id());
+  if (!is_ai_group) {
+    group_header_label_->SetFontList(default_header_font_);
+    SetInteriorMargin(GetHeaderInsets());
+    return;
+  }
+  using browseros::windrise::LayoutMetric;
+  const browseros::windrise::LayoutMetrics& metrics =
+      browseros::windrise::GetLayoutMetrics();
+  const int font_size = metrics.Get(LayoutMetric::kAiSectionHeaderFontSize);
+  group_header_label_->SetFontList(default_header_font_.DeriveWithSizeDelta(
+      font_size - default_header_font_.GetFontSize()));
+  // 行高走偏好：上下留白 = (行高 - 标题实际高) / 2，不硬改子视图高度。
+  const int target_height = metrics.Get(LayoutMetric::kAiSectionHeaderHeight);
+  const int label_height = group_header_label_->GetPreferredSize().height();
+  const int vertical_padding = std::max(0, (target_height - label_height) / 2);
+  SetInteriorMargin(gfx::Insets::TLBR(vertical_padding, GetHorizontalInset(),
+                                      vertical_padding,
+                                      GetHorizontalInset()));
+}
+
 SkColor TabGroupHeaderView::GetBackgroundColor() const {
   if (!GetColorProvider()) {
     return SK_ColorBLACK;
   }
+  // 风桥 AI 用的标签那一段（扩展经 windrise.setAiTabs 指定）不画组底色，
+  // 组头读起来就是这一段的段标题。
+  if (delegate_->IsValid() &&
+      browseros::windrise::AiGroups::Contains(delegate_->GetTabGroup().id())) {
+    return SK_ColorTRANSPARENT;
+  }
   const bool frame_active = GetWidget() && GetWidget()->ShouldPaintAsActive();
   if (delegate_->IsValid() && delegate_->GetTabGroup().is_ephemeral()) {
     const SkColor normal_bg = GetColorProvider()->GetColor(
@@ -639,6 +679,11 @@ SkColor TabGroupHeaderView::GetForegroundColor() const {
   if (!GetColorProvider()) {
     return SK_ColorBLACK;
   }
+  if (delegate_->IsValid() &&
+      browseros::windrise::AiGroups::Contains(delegate_->GetTabGroup().id())) {
+    // 段标题文字色（偏好 browseros.windrise.theme.*.section_header_text）。
+    return GetColorProvider()->GetColor(kColorWindriseSectionHeaderText);
+  }
   if (delegate_->IsValid() && delegate_->GetTabGroup().is_ephemeral()) {
     const bool frame_active = GetWidget() && GetWidget()->ShouldPaintAsActive();
     return GetColorProvider()->GetColor(
