diff --git a/chrome/browser/ui/window_sizer/window_sizer.cc b/chrome/browser/ui/window_sizer/window_sizer.cc
index 4f51764..d07354c 100644
--- a/chrome/browser/ui/window_sizer/window_sizer.cc
+++ b/chrome/browser/ui/window_sizer/window_sizer.cc
@@ -410,6 +410,13 @@ void WindowSizer::AdjustBoundsToBeVisibleOnDisplay(
   if (bounds->y() < work_area.y() || bounds->bottom() > work_area.bottom()) {
     bounds->set_y(work_area.y());
   }
+#elif BUILDFLAG(IS_WIN)
+  // On Windows, keep newly created or restored browser windows strictly inside
+  // the display's work area so the bottom edge (such as the bottom-left AI
+  // status card) cannot slide behind the taskbar.
+  if (!work_area.Contains(*bounds)) {
+    bounds->AdjustToFit(work_area);
+  }
 #else
   // On non-Mac platforms, we are less aggressive about repositioning. Simply
   // ensure that at least kMinVisibleWidth * kMinVisibleHeight is visible or
