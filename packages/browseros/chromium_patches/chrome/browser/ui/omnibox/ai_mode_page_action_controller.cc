diff --git a/chrome/browser/ui/omnibox/ai_mode_page_action_controller.cc b/chrome/browser/ui/omnibox/ai_mode_page_action_controller.cc
index aecbcfb..9c53a42 100644
--- a/chrome/browser/ui/omnibox/ai_mode_page_action_controller.cc
+++ b/chrome/browser/ui/omnibox/ai_mode_page_action_controller.cc
@@ -182,75 +182,8 @@ void AiModePageActionController::NotifyOmniboxTriggeredFeatureService(
 bool AiModePageActionController::ShouldShowPageAction(
     Profile* profile,
     LocationBar& location_bar) {
-  if (!profile->GetPrefs()->GetBoolean(omnibox::kShowAiModeOmniboxButton)) {
-    return false;
-  }
-
-  const auto* aim_eligibility_service =
-      AimEligibilityServiceFactory::GetForProfile(profile);
-  auto* ai_mode_button_service =
-      AiModeButtonServiceFactory::GetForProfile(profile);
-  const auto* template_url_service =
-      TemplateURLServiceFactory::GetForProfile(profile);
-  if (!OmniboxFieldTrial::IsAimOmniboxEntrypointEnabled(aim_eligibility_service,
-                                                        ai_mode_button_service,
-                                                        template_url_service)) {
-    return false;
-  }
-
-  auto* omnibox_controller = location_bar.GetOmniboxController();
-  if (!omnibox_controller) {
-    return false;
-  }
-
-  const OmniboxEditModel* edit_model = omnibox_controller->edit_model();
-
-  // If the user is currently in keyword mode, then suppress the AIM entrypoint.
-  if (edit_model->is_keyword_selected()) {
-    return false;
-  }
-
-  if (omnibox::kShowRhsAimHint.Get()) {
-    return false;
-  }
-
-  // Don't show the AIM entrypoint if the current selected match or typed input
-  // is a URL suggestion.
-  const AutocompleteResult& result =
-      omnibox_controller->autocomplete_controller()->result();
-  const OmniboxPopupSelection selection = edit_model->GetPopupSelection();
-  if (selection.line != OmniboxPopupSelection::kNoMatch &&
-      selection.line < result.size()) {
-    if (!AutocompleteMatch::IsSearchType(
-            result.match_at(selection.line).type)) {
-      return false;
-    }
-  } else if (edit_model->user_input_in_progress() &&
-             !AutocompleteMatch::IsSearchType(
-                 edit_model->CurrentMatch().type)) {
-    return false;
-  }
-
-  // Otherwise, we should show the AIM view if the focus is within any view in
-  // the location bar, including the omnibox, this view, or any other page
-  // action icon views.
-  const bool has_focus = location_bar.IsFocusWithin();
-
-  // TODO(crbug.com/448234135): Remove this logic from the migrated path when
-  // Page Action framework supports suggestion chip queueing.
-  //
-  // Handle the edge case in non-NTP page context with omnibox focus and closed
-  // popup. In this case, we suppress the AIM page action in order to ensure
-  // that it doesn't get visually "sandwiched" in between the other page actions
-  // that show up in this state.
-  const auto page_classification = edit_model->GetPageClassification();
-  if (has_focus && !edit_model->user_input_in_progress() &&
-      !location_bar.GetOmniboxController()->IsPopupOpen() &&
-      !omnibox::IsNTPPage(page_classification)) {
-    return false;
-  }
-
-  return has_focus;
+  // Windrise: suppress upstream Google AI Mode omnibox page action button.
+  return false;
 }

 void AiModePageActionController::OnPageActionIconShown(
