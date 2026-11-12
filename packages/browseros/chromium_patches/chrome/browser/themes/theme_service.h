diff --git a/chrome/browser/themes/theme_service.h b/chrome/browser/themes/theme_service.h
index 9e662c6164..9119745306 100644
--- a/chrome/browser/themes/theme_service.h
+++ b/chrome/browser/themes/theme_service.h
@@ -15,6 +15,7 @@
 #include "base/observer_list.h"
 #include "base/scoped_observation.h"
 #include "base/task/cancelable_task_tracker.h"
+#include "chrome/browser/browseros/core/windrise_theme.h"
 #include "chrome/browser/search/background/theme_delegate.h"
 #include "chrome/browser/themes/browser_theme_provider_delegate.h"
 #include "chrome/browser/themes/theme_helper.h"
@@ -390,6 +391,10 @@ class ThemeService : public KeyedService,
   // color(s), sets browser theme accordingly.
   void HandlePolicyColorUpdate();
 
+  // 风桥外观偏好（browseros.windrise.theme.* / layout.*）变了：颜色缓存已由
+  // ThemePrefObserver 装好新值，这里让所有窗口按新色重绘。
+  void OnWindriseThemePrefsChanged();
+
   // Functions that modify theme prefs.
   void ClearThemePrefs();
   void SetThemePrefsForColor(SkColor color);
@@ -425,6 +430,11 @@ class ThemeService : public KeyedService,
   raw_ptr<Profile> profile_;
   PrefChangeRegistrar pref_change_registrar_;
 
+  // 风桥外观偏好观察者：把 browseros.windrise.theme.* / layout.* 装进进程级
+  // 快照（配色 mixer、布局 provider 都读它）。必须在窗口绘制前构造。
+  std::unique_ptr<browseros::windrise::ThemePrefObserver>
+      windrise_theme_observer_;
+
   const raw_ref<const ThemeHelper> theme_helper_;
   scoped_refptr<CustomThemeSupplier> theme_supplier_;
 
