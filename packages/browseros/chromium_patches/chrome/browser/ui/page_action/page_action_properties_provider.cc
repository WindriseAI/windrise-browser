diff --git a/chrome/browser/ui/page_action/page_action_properties_provider.cc b/chrome/browser/ui/page_action/page_action_properties_provider.cc
index 127fbb81e6..de1ef7917b 100644
--- a/chrome/browser/ui/page_action/page_action_properties_provider.cc
+++ b/chrome/browser/ui/page_action/page_action_properties_provider.cc
@@ -350,6 +350,14 @@ constexpr auto kPageActionProperties = base::MakeFixedFlatMap<
             .element_identifier = kPageActionGmailOtpOptInElementId,
         },
     },
+    {
+        kActionWindriseSlot,
+        {
+            .histogram_name = "WindriseSlot",
+            .type = PageActionIconType::kWindriseSlot,
+            .element_identifier = kWindriseSlotPageActionElementId,
+        },
+    },
 });
 // LINT.ThenChange(//components/browser_apis/ui_controllers/toolbar/toolbar_ui_api_data_model.mojom:PageActionId,//chrome/browser/ui/page_action/action_ids.h:kActionIds)
 
