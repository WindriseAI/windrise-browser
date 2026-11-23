diff --git a/chrome/browser/ui/window_sizer/window_sizer_unittest.cc b/chrome/browser/ui/window_sizer/window_sizer_unittest.cc
--- a/chrome/browser/ui/window_sizer/window_sizer_unittest.cc
+++ b/chrome/browser/ui/window_sizer/window_sizer_unittest.cc
@@ -538,6 +538,29 @@
     EXPECT_EQ(gfx::Rect(0 /* not -100 */, 50, 2000, 400), window_bounds);
   }
 }
+#elif BUILDFLAG(IS_WIN)
+TEST(WindowSizerTest, WindowsClampsRestoredBoundsForEveryTaskbarEdge) {
+  for (const gfx::Rect& work_area :
+       {taskbar_top_work_area, taskbar_bottom_work_area, taskbar_left_work_area,
+        taskbar_right_work_area}) {
+    SCOPED_TRACE(work_area.ToString());
+    const gfx::Rect bounds = WindowSizerTestUtil()
+                                 .WithMonitorBounds(p1024x768)
+                                 .WithMonitorWorkArea(work_area)
+                                 .WithPersistedBounds(p1024x768)
+                                 .GetWindowBounds();
+    EXPECT_EQ(work_area, bounds);
+  }
+}
+
+TEST(WindowSizerTest, WindowsKeepsExplicitOffscreenBounds) {
+  const gfx::Rect hidden(2400, -700, 320, 240);
+  EXPECT_EQ(hidden, WindowSizerTestUtil()
+                        .WithMonitorBounds(p1024x768, right_s1024x768)
+                        .WithMonitorWorkArea(taskbar_top_work_area)
+                        .WithSpecifiedBounds(hidden)
+                        .GetWindowBounds());
+}
 #else
 TEST(WindowSizerTest, LastWindowOffscreenWithNonAggressiveRepositioning) {
   {  // taskbar on left.
