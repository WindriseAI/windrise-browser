diff --git a/ui/views/controls/menu/menu_controller.h b/ui/views/controls/menu/menu_controller.h
index 655814df01..56a199d74d 100644
--- a/ui/views/controls/menu/menu_controller.h
+++ b/ui/views/controls/menu/menu_controller.h
@@ -167,6 +167,11 @@ class VIEWS_EXPORT MenuController final : public gfx::AnimationDelegate,
 
   bool for_drop() const { return for_drop_; }
 
+  // Windrise motion-1115 (N10): whether the current menu was opened with the
+  // mouse. MenuHost gives mouse-opened root menus a 120ms fade-in; menus
+  // opened from the keyboard (and submenus) appear instantly.
+  bool windrise_opened_by_mouse() const { return windrise_opened_by_mouse_; }
+
   // Whether or not drag operation is in progress.
   bool drag_in_progress() const { return drag_in_progress_; }
 
@@ -875,6 +880,7 @@ class VIEWS_EXPORT MenuController final : public gfx::AnimationDelegate,
   // If a mouse press triggered this menu, this will have its location (in
   // screen coordinates). Otherwise this will be (0, 0).
   gfx::Point menu_start_mouse_press_loc_;
+  bool windrise_opened_by_mouse_ = false;
 
   // Set to the location, relative to the root menu item's widget, of the mouse
   // cursor if any submenu is opened while the cursor is over that menu. This is
