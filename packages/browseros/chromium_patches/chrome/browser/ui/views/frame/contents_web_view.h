diff --git a/chrome/browser/ui/views/frame/contents_web_view.h b/chrome/browser/ui/views/frame/contents_web_view.h
index af18353b9d..e5c763faea 100644
--- a/chrome/browser/ui/views/frame/contents_web_view.h
+++ b/chrome/browser/ui/views/frame/contents_web_view.h
@@ -58,7 +58,14 @@ class ContentsWebView : public views::WebView,
   void OnLetterboxingChanged() override;
   void SetWebContents(content::WebContents* web_contents) override;
+  void AddedToWidget() override;
 
   // content::WebContentsObserver overrides:
+  void ReadyToCommitNavigation(content::NavigationHandle* navigation) override;
+  // Windrise: a main frame swapped in by a navigation (e.g. the new tab page
+  // committing in its own process) gets the background color before its first
+  // frame; RenderViewReady() is not sent for it.
+  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
+                              content::RenderFrameHost* new_host) override;
   // Overridden to track physical interactions (mouse/touch) on the WebContents.
   // This allows the browser to force focus synchronization in split view even
   // when native OS focus gets stuck on a different window (like a permission
@@ -75,6 +82,6 @@ class ContentsWebView : public views::WebView,
   void DestroyClonedLayer() override;

  private:
-  void UpdateBackgroundColor();
+  void UpdateBackgroundColor(const GURL* commit_url = nullptr);
   std::unique_ptr<StatusBubbleViews> status_bubble_;
   std::unique_ptr<WebContentsCloseHandler> web_contents_close_handler_;
