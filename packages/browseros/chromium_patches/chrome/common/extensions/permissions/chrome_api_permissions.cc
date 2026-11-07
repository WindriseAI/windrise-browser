diff --git a/chrome/common/extensions/permissions/chrome_api_permissions.cc b/chrome/common/extensions/permissions/chrome_api_permissions.cc
index 2bb5375..9fb4de5 100644
--- a/chrome/common/extensions/permissions/chrome_api_permissions.cc
+++ b/chrome/common/extensions/permissions/chrome_api_permissions.cc
@@ -65,6 +65,7 @@ constexpr APIPermissionInfo::InitInfo permissions_to_register[] = {
     {APIPermissionID::kBookmark, "bookmarks"},
     {APIPermissionID::kBrailleDisplayPrivate, "brailleDisplayPrivate",
      APIPermissionInfo::kFlagCannotBeOptional},
+    {APIPermissionID::kBrowserOS, "browserOS"},
     {APIPermissionID::kBrowsingData, "browsingData",
      APIPermissionInfo::kFlagDoesNotRequireManagedSessionFullLoginWarning},
     {APIPermissionID::kContentSettings, "contentSettings"},
