diff --git a/chrome/browser/ui/window_sizer/window_sizer.cc b/chrome/browser/ui/window_sizer/window_sizer.cc
--- a/chrome/browser/ui/window_sizer/window_sizer.cc
+++ b/chrome/browser/ui/window_sizer/window_sizer.cc
@@ -274,8 +274,16 @@
   // of the anchor window. Note: AdjustBoundsToBeVisibleOnMonitorContaining
   // does not exactly what we want: It makes only sure that "a minimal part"
   // is visible on the screen.
-  gfx::Rect work_area =
-      display::Screen::Get()->GetDisplayMatching(*bounds).work_area();
+  const display::Display display =
+      display::Screen::Get()->GetDisplayMatching(*bounds);
+#if BUILDFLAG(IS_WIN)
+  // An explicit placement outside every monitor is used to keep a warmup
+  // window hidden. Saved placements still use normal display recovery.
+  if (!bounds->Intersects(display.bounds())) {
+    return;
+  }
+#endif
+  gfx::Rect work_area = display.work_area();

   AdjustWorkAreaForPlatform(work_area);

@@ -394,6 +402,13 @@
       !work_area.Contains(*bounds)) {
     bounds->AdjustToFit(work_area);
   }
+
+#if BUILDFLAG(IS_WIN)
+  if (work_area != display.bounds() && bounds->Intersects(display.bounds())) {
+    bounds->AdjustToFit(work_area);
+    return;
+  }
+#endif

 #if BUILDFLAG(IS_MAC)
   bounds->set_height(std::min(work_area.height(), bounds->height()));
