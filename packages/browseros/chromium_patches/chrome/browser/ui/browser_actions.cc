diff --git a/chrome/browser/ui/browser_actions.cc b/chrome/browser/ui/browser_actions.cc
index d716fe0590..b343774766 100644
--- a/chrome/browser/ui/browser_actions.cc
+++ b/chrome/browser/ui/browser_actions.cc
@@ -20,6 +20,7 @@
 #include "base/metrics/user_metrics_action.h"
 #include "base/notreached.h"
 #include "base/types/to_address.h"
+#include "base/values.h"
 #include "build/branding_buildflags.h"
 #include "chrome/app/chrome_command_ids.h"
 #include "chrome/app/vector_icons/vector_icons.h"
@@ -233,6 +234,9 @@
 #include "components/sync/base/features.h"
 #include "components/tabs/public/tab_interface.h"
 #include "content/public/browser/page_navigator.h"
+#include "extensions/browser/event_router.h"
+#include "extensions/browser/extension_event_histogram_value.h"
+#include "chrome/browser/extensions/extension_tab_util.h"
 #include "extensions/common/extension_urls.h"
 #if BUILDFLAG(IS_CHROMEOS)
 #include "chrome/browser/ui/ash/multi_user/multi_user_context_menu.h"
@@ -1581,6 +1585,36 @@ void BrowserActions::InitializeChromeMenuActions() {
           vector_icons::kGTranslateIcon)
           .Build());
 
+  // 风桥通用地址栏插槽：文字与显隐由插件通过 browserOS.setAddressBarSlot
+  // 控制。点击时把当前标签页 id 通过扩展事件发出去。
+  root_action_item_->AddChild(
+      actions::ActionItem::Builder(
+          base::BindRepeating(
+              [](BrowserWindowInterface* bwi, actions::ActionItem* item,
+                 actions::ActionInvocationContext context) {
+                tabs::TabInterface* tab = bwi->GetActiveTabInterface();
+                if (!tab) {
+                  return;
+                }
+                auto* event_router =
+                    extensions::EventRouter::Get(bwi->GetProfile());
+                if (!event_router) {
+                  return;
+                }
+                auto args = base::MakeRefCounted<extensions::EventArgs>();
+                args->data.Append(extensions::ExtensionTabUtil::GetTabId(
+                    tab->GetContents()));
+                event_router->BroadcastEvent(std::make_unique<extensions::Event>(
+                    extensions::events::UNKNOWN,
+                    "browserOS.onAddressBarSlotClicked", std::move(args),
+                    bwi->GetProfile()));
+              },
+              bwi))
+          .SetActionId(kActionWindriseSlot)
+          .SetImage(ui::ImageModel::FromVectorIcon(
+              kWindriseFlashIcon, ui::kColorIcon))
+          .Build());
+
   root_action_item_->AddChild(
       actions::ActionItem::Builder(
           base::BindRepeating(
