diff --git a/ui/views/bubble/bubble_dialog_delegate_view.h b/ui/views/bubble/bubble_dialog_delegate_view.h
index c63a5501d9..062162b5b3 100644
--- a/ui/views/bubble/bubble_dialog_delegate_view.h
+++ b/ui/views/bubble/bubble_dialog_delegate_view.h
@@ -64,6 +64,7 @@ class ExtensionInstallDialogView;
 class ExtensionPopup;
 class ExtensionsMenuView;
 class FlyingIndicator;
+class WindriseAiPanelView;
 class GlobalErrorBubbleView;
 class HomePageUndoBubble;
 class MediaDialogView;
@@ -921,6 +922,8 @@ class VIEWS_EXPORT BubbleDialogDelegateView : public View,
   friend class ::ProfileMenuViewBase;
   friend class ::RemoveSuggestionBubbleDialogDelegateView;
   friend class ::StoragePressureBubbleView;
+  // 风桥 N7：左下「已连接 N 个 AI」面板，走 CreatePassKey() 构造。
+  friend class ::WindriseAiPanelView;
   friend class ::TabGroupEditorBubbleView;
   friend class ::TabHoverCardBubbleView;
   friend class ::TestBubbleView;
