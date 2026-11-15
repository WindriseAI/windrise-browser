diff --git a/ui/views/controls/menu/menu_controller.cc b/ui/views/controls/menu/menu_controller.cc
index 20ad3d05bb..3f06f479e1 100644
--- a/ui/views/controls/menu/menu_controller.cc
+++ b/ui/views/controls/menu/menu_controller.cc
@@ -793,6 +793,11 @@ void MenuController::Run(Widget* parent,
     }
   }
 
+  // Windrise motion-1115 (N10): see windrise_opened_by_mouse().
+  windrise_opened_by_mouse_ =
+      source_type == ui::mojom::MenuSourceType::kMouse ||
+      (event && event->type() == ui::EventType::kMousePressed);
+
   // If we are already showing, this new menu is being nested. Such as context
   // menus on top of normal menus.
   if (showing_) {
