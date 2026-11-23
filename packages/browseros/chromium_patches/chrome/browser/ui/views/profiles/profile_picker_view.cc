diff --git a/chrome/browser/ui/views/profiles/profile_picker_view.cc b/chrome/browser/ui/views/profiles/profile_picker_view.cc
index 9380cb0..d0eae60 100644
--- a/chrome/browser/ui/views/profiles/profile_picker_view.cc
+++ b/chrome/browser/ui/views/profiles/profile_picker_view.cc
@@ -68,13 +68,18 @@
 #include "content/public/browser/navigation_controller.h"
 #include "content/public/browser/navigation_handle.h"
 #include "content/public/browser/render_frame_host.h"
+#include "content/public/browser/render_widget_host_view.h"
 #include "content/public/browser/web_contents.h"
 #include "content/public/common/url_constants.h"
 #include "google_apis/gaia/gaia_id.h"
+#include "third_party/skia/include/core/SkColor.h"
 #include "ui/base/l10n/l10n_util.h"
 #include "ui/base/metadata/metadata_impl_macros.h"
 #include "ui/base/page_transition_types.h"
 #include "ui/base/window_open_disposition.h"
+#include "ui/display/screen.h"
+#include "ui/native_theme/native_theme.h"
+#include "ui/views/background.h"
 #include "ui/views/controls/webview/webview.h"
 #include "ui/views/layout/fill_layout.h"
 #include "ui/views/view.h"
@@ -108,6 +113,11 @@ constexpr int kWindowWidth = 1024;
 constexpr int kWindowHeight = 758;
 constexpr int kWindowWidthIncreased = 1600;
 constexpr int kWindowHeightIncreased = 1000;
+constexpr int kWindriseOnboardingWidth = 960;
+constexpr int kWindriseOnboardingHeight = 640;
+constexpr int kWindriseWorkAreaMargin = 64;
+constexpr SkColor kWindriseOnboardingBgLight = SkColorSetRGB(0xD5, 0xDA, 0xDF);
+constexpr SkColor kWindriseOnboardingBgDark = SkColorSetRGB(0x3B, 0x42, 0x4A);

 constexpr float kMaxRatioOfWorkArea = 0.9;

@@ -360,6 +370,7 @@ void ProfilePickerView::ShowScreen(
   if (GetWidget()) {
     contents->SetColorProviderSource(GetWidget());
   }
+  ApplyWindriseOnboardingBackground(contents);

   contents->GetController().LoadURL(url, content::Referrer(),
                                     ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
@@ -501,9 +512,41 @@ bool ProfilePickerView::AreNativeToolbarSigninButtonsVisibleForTesting() const {
 }

 SkColor ProfilePickerView::GetPreferredBackgroundColor() const {
+  if (params_.entry_point() == ProfilePicker::EntryPoint::kFirstRun) {
+    return GetWindriseOnboardingBackgroundColor();
+  }
   return GetColorProvider()->GetColor(kColorToolbar);
 }

+SkColor ProfilePickerView::GetWindriseOnboardingBackgroundColor() const {
+  const bool dark =
+      GetWidget()
+          ? ShouldUseDarkColors()
+          : (ui::NativeTheme::GetInstanceForNativeUi()
+                 ->preferred_color_scheme() ==
+             ui::NativeTheme::PreferredColorScheme::kDark);
+  return dark ? kWindriseOnboardingBgDark : kWindriseOnboardingBgLight;
+}
+
+void ProfilePickerView::ApplyWindriseOnboardingBackground(
+    content::WebContents* contents) {
+  if (params_.entry_point() != ProfilePicker::EntryPoint::kFirstRun) {
+    return;
+  }
+  const SkColor bg = GetWindriseOnboardingBackgroundColor();
+  SetBackground(views::CreateSolidBackground(bg));
+  if (web_view_) {
+    web_view_->SetBackground(views::CreateSolidBackground(bg));
+  }
+  if (contents) {
+    contents->SetPageBaseBackgroundColor(bg);
+    if (content::RenderWidgetHostView* rwhv =
+            contents->GetRenderWidgetHostView()) {
+      rwhv->SetBackgroundColor(bg);
+    }
+  }
+}
+
 bool ProfilePickerView::CanNavigateBack() const {
   return flow_controller_ && flow_controller_->CanNavigateBack();
 }
@@ -635,6 +678,7 @@ void ProfilePickerView::Init(Profile* picker_profile) {
       (picker_profile ? picker_profile->GetPath().AsUTF8Unsafe() : ""));
   contents_ = content::WebContents::Create(
       content::WebContents::CreateParams(picker_profile));
+  ApplyWindriseOnboardingBackground(contents_.get());
   contents_->SetDelegate(this);
   SetAcceleratorHandling(
       views::WebContentsAcceleratorHandling::kProcessBeforeRenderer);
@@ -670,9 +714,29 @@ void ProfilePickerView::Init(Profile* picker_profile) {
   flow_controller_ = CreateFlowController(picker_profile, GetClearClosure());

   toolbar_ = AddChildView(flow_controller_->CreateToolbarBuilder().Build());
+  if (params_.entry_point() == ProfilePicker::EntryPoint::kFirstRun) {
+    toolbar_->SetVisible(false);
+  }

   // The widget is owned by the native widget.
   new ProfilePickerWidget(this);
+  if (params_.entry_point() == ProfilePicker::EntryPoint::kFirstRun &&
+      GetWidget()) {
+    gfx::Rect work_area = GetWidget()->GetWorkAreaBoundsInScreen();
+    if (work_area.IsEmpty()) {
+      work_area = display::Screen::Get()->GetPrimaryDisplay().work_area();
+    }
+    gfx::Size size = CalculatePreferredSize({});
+    gfx::Rect bounds(
+        work_area.x() + std::max(0, (work_area.width() - size.width()) / 2),
+        work_area.y() + std::max(0, (work_area.height() - size.height()) / 2),
+        size.width(), size.height());
+    if (!work_area.IsEmpty()) {
+      bounds.AdjustToFit(work_area);
+    }
+    GetWidget()->SetBounds(bounds);
+    ApplyWindriseOnboardingBackground(contents_.get());
+  }

 #if BUILDFLAG(IS_WIN)
   // Set the app id for the user manager to the app id of its parent.
@@ -809,16 +873,36 @@ std::u16string ProfilePickerView::GetAccessibleWindowTitle() const {

 gfx::Size ProfilePickerView::CalculatePreferredSize(
     const views::SizeBounds& available_size) const {
-  gfx::Size preferred_size = gfx::Size(
-      switches::IsPreFirstRunDesktopRefreshEnabled() ? kWindowWidthIncreased
-                                                     : kWindowWidth,
-      switches::IsPreFirstRunDesktopRefreshEnabled() ? kWindowHeightIncreased
-                                                     : kWindowHeight);
+  const bool is_first_run =
+      params_.entry_point() == ProfilePicker::EntryPoint::kFirstRun;
+  gfx::Size preferred_size =
+      is_first_run
+          ? gfx::Size(kWindriseOnboardingWidth, kWindriseOnboardingHeight)
+          : gfx::Size(switches::IsPreFirstRunDesktopRefreshEnabled()
+                          ? kWindowWidthIncreased
+                          : kWindowWidth,
+                      switches::IsPreFirstRunDesktopRefreshEnabled()
+                          ? kWindowHeightIncreased
+                          : kWindowHeight);
   gfx::Size work_area_size = GetWidget()->GetWorkAreaBoundsInScreen().size();
+  if (work_area_size.IsEmpty()) {
+    work_area_size =
+        display::Screen::Get()->GetPrimaryDisplay().work_area().size();
+  }
   // Keep the window smaller then |work_area_size| so that it feels more like a
   // dialog then like the actual Chrome window.
   gfx::Size max_dialog_size = ScaleToFlooredSize(
       work_area_size, kMaxRatioOfWorkArea, kMaxRatioOfWorkArea);
+  if (is_first_run && !work_area_size.IsEmpty()) {
+    max_dialog_size.SetToMin(gfx::Size(
+        std::min(work_area_size.width(),
+                 std::max(640, work_area_size.width() - kWindriseWorkAreaMargin)),
+        std::min(
+            work_area_size.height(),
+            std::max(480, work_area_size.height() - kWindriseWorkAreaMargin))));
+  }
-  preferred_size.SetToMin(max_dialog_size);
+  if (!max_dialog_size.IsEmpty()) {
+    preferred_size.SetToMin(max_dialog_size);
+  }
   return preferred_size;
 }
@@ -889,7 +973,18 @@ bool ProfilePickerView::GetAcceleratorForCommandId(
 void ProfilePickerView::Layout(PassKey) {
   LayoutSuperclass<views::WidgetDelegateView>(this);
   CHECK(toolbar_);
-  toolbar_->SetBounds(0, 0, width(), toolbar_->GetPreferredSize().height());
+  if (toolbar_->GetVisible()) {
+    toolbar_->SetBounds(0, 0, width(), toolbar_->GetPreferredSize().height());
+  } else {
+    toolbar_->SetBounds(0, 0, 0, 0);
+  }
+}
+
+void ProfilePickerView::OnThemeChanged() {
+  views::WidgetDelegateView::OnThemeChanged();
+  ApplyWindriseOnboardingBackground(
+      web_view_ && web_view_->GetWebContents() ? web_view_->GetWebContents()
+                                               : contents_.get());
 }

 void ProfilePickerView::BuildLayout() {
@@ -897,6 +992,7 @@ void ProfilePickerView::BuildLayout() {

   auto web_view = std::make_unique<views::WebView>();
   web_view_ = AddChildView(std::move(web_view));
+  ApplyWindriseOnboardingBackground(nullptr);

   web_contents_attached_subscription_ =
       web_view_->AddWebContentsAttachedCallback(base::BindRepeating(
@@ -912,5 +1008,6 @@ void ProfilePickerView::ShowScreenFinished(
     show_screen_finished_observer_.reset();
   }

+  ApplyWindriseOnboardingBackground(contents);
   web_view_->SetWebContents(contents);
   contents->Focus();
