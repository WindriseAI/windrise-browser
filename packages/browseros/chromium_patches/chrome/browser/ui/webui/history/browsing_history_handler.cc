diff --git a/chrome/browser/ui/webui/history/browsing_history_handler.cc b/chrome/browser/ui/webui/history/browsing_history_handler.cc
index 2ca7d2b..0aa55dc 100644
--- a/chrome/browser/ui/webui/history/browsing_history_handler.cc
+++ b/chrome/browser/ui/webui/history/browsing_history_handler.cc
@@ -117,6 +117,12 @@ namespace {
 constexpr int kHistorySyncPromoShownThreshold = 5;
 constexpr base::TimeDelta kHistorySyncPromoCooldown = base::Days(7);
 
+// Windrise: this build has no Google API key, so signing in and syncing
+// history cannot work and the history sync promo would be an empty shell.
+// Keep it off. The remaining logic stays so that every constant and helper
+// above and below is still referenced.
+constexpr bool kHistorySyncPromoSupported = false;
+
 history::mojom::AccountInfoPtr CreateAccountInfoDataMojo(
     const AccountInfo& info) {
   auto account_info_mojo = history::mojom::AccountInfo::New();
@@ -728,6 +734,11 @@ void BrowsingHistoryHandler::IncrementHistoryPageHistorySyncPromoShownCount() {}
 #elif !BUILDFLAG(IS_CHROMEOS)
 void BrowsingHistoryHandler::ShouldShowHistoryPageHistorySyncPromo(
     ShouldShowHistoryPageHistorySyncPromoCallback callback) {
+  if (!kHistorySyncPromoSupported) {
+    std::move(callback).Run(false);
+    return;
+  }
+
   const int promo_shown_count = GetHistoryPageHistorySyncPromoShownCount();
 
   // If the promo has been shown more than the threshold, the promo should not
