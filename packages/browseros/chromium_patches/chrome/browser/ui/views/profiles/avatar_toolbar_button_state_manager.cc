diff --git a/chrome/browser/ui/views/profiles/avatar_toolbar_button_state_manager.cc b/chrome/browser/ui/views/profiles/avatar_toolbar_button_state_manager.cc
index a7100cf..af83ac9 100644
--- a/chrome/browser/ui/views/profiles/avatar_toolbar_button_state_manager.cc
+++ b/chrome/browser/ui/views/profiles/avatar_toolbar_button_state_manager.cc
@@ -2446,11 +2446,17 @@ void AvatarToolbarButtonStateManager::CreateStatesAndListeners(
             /*state_observer=*/this);
 
 #if BUILDFLAG(ENABLE_DICE_SUPPORT)
-    RegisterObserver(static_cast<Observer*>(
-        &PromoStateProviderCoordinator::GetOrCreateForProfile(*profile)));
-    states_[ButtonState::kPromo] =
-        std::make_unique<PromoStateProvider>(browser,
-                                             /*state_observer=*/this);
+    // BrowserOS 补丁在这里加了 Dice 开关保护；158 把这里改成无条件创建，
+    // 但 AccountConsistencyModeManager 的 include 仍在，故保留补丁的条件。
+    if (AccountConsistencyModeManager::IsDiceEnabledForProfile(profile) &&
+        (syncer::IsReplaceSyncPromosWithSignInPromosEnabled() ||
+         switches::IsAvatarSyncPromoFeatureEnabled())) {
+      RegisterObserver(static_cast<Observer*>(
+          &PromoStateProviderCoordinator::GetOrCreateForProfile(*profile)));
+      states_[ButtonState::kPromo] =
+          std::make_unique<PromoStateProvider>(browser,
+                                               /*state_observer=*/this);
+    }
 
     // Contains both Work and School.
     states_[ButtonState::kManagement] =
