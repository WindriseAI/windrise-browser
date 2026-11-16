diff --git a/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.cc b/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.cc
index 4eb9220..f40df30 100644
--- a/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.cc
+++ b/chrome/browser/ui/tabs/vertical_tab_strip_state_controller_impl.cc
@@ -12,6 +12,7 @@
 #include "base/strings/string_number_conversions.h"
 #include "base/strings/to_string.h"
 #include "base/timer/timer.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
 #include "chrome/app/vector_icons/vector_icons.h"
 #include "chrome/browser/profiles/profile.h"
 #include "chrome/browser/sessions/session_service.h"
@@ -55,6 +56,8 @@ VerticalTabStripStateControllerImpl::VerticalTabStripStateControllerImpl(
       session_service_(session_service),
       session_id_(session_id),
       browser_window_(browser_window) {
+  browseros::SyncVerticalTabsPref(pref_service_);
+
   pref_change_registrar_.Init(pref_service_);
 
   is_vertical_tabs_enabled_ =
@@ -74,6 +77,28 @@ VerticalTabStripStateControllerImpl::VerticalTabStripStateControllerImpl(
           &VerticalTabStripStateControllerImpl::OnExpandOnHoverEnabledChanged,
           base::Unretained(this)));
 
+  pref_change_registrar_.Add(
+      browseros::prefs::kVerticalTabsEnabled,
+      base::BindRepeating(
+          [](PrefService* ps) {
+            ps->SetBoolean(
+                prefs::kVerticalTabsEnabled,
+                ps->GetBoolean(browseros::prefs::kVerticalTabsEnabled));
+          },
+          base::Unretained(pref_service_)));
+
+  // Windrise starts collapsed. Only apply this while the persisted state is
+  // still the upstream default: once the user expands or collapses the strip,
+  // Chromium writes that choice and it must win on later launches.
+  const PrefService::Preference* collapsed_pref =
+      pref_service_->FindPreference(prefs::kVerticalTabsCollapsedState);
+  if (restored_state_collapsed.has_value() &&
+      !restored_state_collapsed.value() && collapsed_pref &&
+      collapsed_pref->IsDefaultValue() &&
+      browseros::ShouldStartVerticalTabsCollapsed(pref_service_)) {
+    restored_state_collapsed = true;
+  }
+
   if (restored_state_collapsed.has_value()) {
     SetCollapsed(restored_state_collapsed.value());
   }
@@ -192,6 +217,9 @@ VerticalTabStripStateControllerImpl::GetCollapseState() const {
 }
 
 void VerticalTabStripStateControllerImpl::RequestCollapse(bool collapse) {
+  // 风桥 L11：之后任何一次展开 / 收起请求（用户点的）都不再是那次自动收起。
+  // 自动收起自己也走这里，它在调用之后才把这个标记立起来。
+  windrise_auto_collapse_pending_ = false;
   if (!delegate_) {
     return;
   }
@@ -294,6 +322,16 @@ void VerticalTabStripStateControllerImpl::OnExpandOnHoverEnabledChanged() {
 
 void VerticalTabStripStateControllerImpl::SetCollapsed(bool collapsed) {
   if (state_.collapsed != collapsed) {
+    if (windrise_narrow_window_) {
+      if (windrise_auto_collapse_pending_ && collapsed) {
+        // 窄窗自动收起落地：不算用户的选择。
+        windrise_auto_collapse_pending_ = false;
+      } else {
+        // 窄着的时候用户自己点了展开 / 收起：这是用户的新选择。
+        windrise_auto_collapse_pending_ = false;
+        windrise_user_collapsed_ = collapsed;
+      }
+    }
     state_.collapsed = collapsed;
     NotifyCollapseChanged();
     if (auto* browser_element_views =
@@ -325,8 +363,9 @@ void VerticalTabStripStateControllerImpl::OnLockDestroyed() {
 
 void VerticalTabStripStateControllerImpl::UpdateSessionService() {
   if (session_service_ && !browser_collection_observation_.IsObserving()) {
-    session_service_->AddWindowExtraData(session_id_, kCollapsedKey,
-                                         base::ToString(state_.collapsed));
+    session_service_->AddWindowExtraData(
+        session_id_, kCollapsedKey,
+        base::ToString(GetWindrisePersistedCollapsed()));
     session_service_->AddWindowExtraData(
         session_id_, kUncollapsedWidthKey,
         base::NumberToString(state_.uncollapsed_width));
@@ -336,12 +375,43 @@ void VerticalTabStripStateControllerImpl::UpdateSessionService() {
 void VerticalTabStripStateControllerImpl::UpdatePrefService() {
   if (pref_service_) {
     pref_service_->SetBoolean(prefs::kVerticalTabsCollapsedState,
-                              state_.collapsed);
+                              GetWindrisePersistedCollapsed());
     pref_service_->SetInteger(prefs::kVerticalTabsUncollapsedWidth,
                               state_.uncollapsed_width);
   }
 }
 
+bool VerticalTabStripStateControllerImpl::GetWindrisePersistedCollapsed()
+    const {
+  return windrise_narrow_window_ ? windrise_user_collapsed_ : state_.collapsed;
+}
+
+void VerticalTabStripStateControllerImpl::SetWindriseNarrowWindow(
+    bool narrow) {
+  if (windrise_narrow_window_ == narrow) {
+    return;
+  }
+  if (narrow) {
+    // 进窄窗：记下用户此刻的选择，再自动收起（走侧栏原有的收起动画）。
+    windrise_user_collapsed_ = state_.collapsed;
+    windrise_narrow_window_ = true;
+    if (!state_.collapsed) {
+      RequestCollapse(true);
+      // 收起动画落地（SetCollapsed）时认出是这次自动收起。减少动态效果时可能
+      // 在 RequestCollapse 里就同步落地了，那就不用再等。
+      windrise_auto_collapse_pending_ = !state_.collapsed;
+    }
+    return;
+  }
+  // 宽回来：先退出窄窗状态，再恢复用户自己的选择；恢复时写进偏好的就是用户
+  // 原来的值。
+  windrise_narrow_window_ = false;
+  windrise_auto_collapse_pending_ = false;
+  if (state_.collapsed != windrise_user_collapsed_) {
+    RequestCollapse(windrise_user_collapsed_);
+  }
+}
+
 void VerticalTabStripStateControllerImpl::UpdateCollapseActionItem() {
   const bool is_collapsed =
       GetCollapseState() != VerticalTabStripCollapseState::kExpanded;
