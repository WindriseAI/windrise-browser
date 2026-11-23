diff --git a/chrome/browser/ui/views/profiles/profile_picker_view.h b/chrome/browser/ui/views/profiles/profile_picker_view.h
index fab592e..10075d2 100644
--- a/chrome/browser/ui/views/profiles/profile_picker_view.h
+++ b/chrome/browser/ui/views/profiles/profile_picker_view.h
@@ -112,6 +112,7 @@ class ProfilePickerView
   views::ClientView* CreateClientView(views::Widget* widget) override;
   views::View* GetContentsView() override;
   void Layout(PassKey) override;
+  void OnThemeChanged() override;
   std::u16string GetAccessibleWindowTitle() const override;
   gfx::Size CalculatePreferredSize(
       const views::SizeBounds& available_size) const override;
@@ -225,6 +226,10 @@ class ProfilePickerView
   // Builds the views hierarchy.
   void BuildLayout();

+  // Returns Windrise onboarding first-frame background (#D5DADF / #3B424A).
+  SkColor GetWindriseOnboardingBackgroundColor() const;
+  void ApplyWindriseOnboardingBackground(content::WebContents* contents);
+
   void ShowScreenFinished(
       content::WebContents* contents,
       base::OnceClosure navigation_finished_closure = base::OnceClosure());
