diff --git a/chrome/browser/extensions/extension_context_menu_model.cc b/chrome/browser/extensions/extension_context_menu_model.cc
index 2ea4a34..06f12da 100644
--- a/chrome/browser/extensions/extension_context_menu_model.cc
+++ b/chrome/browser/extensions/extension_context_menu_model.cc
@@ -16,6 +16,7 @@
 #include "base/notreached.h"
 #include "base/strings/utf_string_conversions.h"
 #include "base/types/pass_key.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
 #include "chrome/browser/extensions/api/side_panel/side_panel_service.h"
 #include "chrome/browser/extensions/context_menu_matcher.h"
 #include "chrome/browser/extensions/extension_management.h"
@@ -892,7 +893,11 @@ void ExtensionContextMenuModel::InitMenuWithFeature(
 
   // Controls section.
   bool has_options_page = OptionsPageInfo::HasOptionsPage(extension);
-  bool can_uninstall_extension = !is_component_ && !is_required_by_policy;
+  // Product-managed extensions are maintained by BrowserOS, so the toolbar
+  // menu must not offer removal alongside ordinary user-installed extensions.
+  bool can_uninstall_extension =
+      !is_component_ && !is_required_by_policy &&
+      !browseros::IsActiveBrowserOSExtension(extension->id());
   if (can_show_icon_in_toolbar || has_options_page || can_uninstall_extension) {
     AddSeparator(ui::NORMAL_SEPARATOR);
   }
