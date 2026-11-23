diff --git a/chrome/browser/themes/theme_service.cc b/chrome/browser/themes/theme_service.cc
index cc3523cb5c..bc51ee85ba 100644
--- a/chrome/browser/themes/theme_service.cc
+++ b/chrome/browser/themes/theme_service.cc
@@ -34,6 +34,7 @@
 #include "base/trace_event/trace_event.h"
 #include "build/build_config.h"
 #include "chrome/browser/browser_process.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
 #include "chrome/browser/infobars/browser_infobar_manager.h"
 #include "chrome/browser/infobars/infobar_features.h"
 #include "chrome/browser/infobars/infobar_spec.h"
@@ -346,6 +347,22 @@ ThemeService::~ThemeService() = default;
 void ThemeService::Init() {
   theme_helper_->DCheckCalledOnValidSequence();
 
+  // Seed the BrowserOS default before ThemeService builds its initial theme.
+  // SyncDefaultTheme only seeds the color while its preference is default.
+  browseros::SyncDefaultTheme(profile_->GetPrefs());
+
+  // 风桥外观数值：构造观察者时先装一次快照（第一帧就用偏好里的颜色和尺寸），
+  // 之后任一外观偏好变化都会重装快照并回调重绘。
+  windrise_theme_observer_ =
+      std::make_unique<browseros::windrise::ThemePrefObserver>(
+          profile_->GetPrefs(),
+          base::BindRepeating(&ThemeService::OnWindriseThemePrefsChanged,
+                              base::Unretained(this)),
+          // 外观快照是进程级的，只让常规 profile 当属主；访客、系统（选人页）
+          // profile 不参与，多个 profile 并存时也只有第一个的偏好生效。
+          /*take_ownership=*/profile_->IsRegularProfile() &&
+              !profile_->IsGuestSession() && !profile_->IsSystemProfile());
+
   InitFromPrefs();
 
   // ThemeObserver should be constructed before calling
@@ -1046,6 +1063,15 @@ void ThemeService::NotifyThemeChanged() {
   }
 }
 
+void ThemeService::OnWindriseThemePrefsChanged() {
+  // 新的颜色 / 尺寸已经由 ThemePrefObserver 装进快照。颜色提供者按
+  // ColorProviderKey 缓存，必须先清缓存，下一次取值才会重新跑 mixer；
+  // NotifyOnNativeThemeUpdated 清缓存并把「原生主题变了」发给所有窗口，
+  // 窗口据此重取颜色重绘。
+  ui::NativeTheme::GetInstanceForNativeUi()->NotifyOnNativeThemeUpdated();
+  NotifyThemeChanged();
+}
+
 void ThemeService::FixInconsistentPreferencesIfNeeded() {}
 
 void ThemeService::SwapThemeSupplier(
