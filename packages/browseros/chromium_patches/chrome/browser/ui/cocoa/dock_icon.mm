diff --git a/chrome/browser/ui/cocoa/dock_icon.mm b/chrome/browser/ui/cocoa/dock_icon.mm
index 4813226..999a749 100644
--- a/chrome/browser/ui/cocoa/dock_icon.mm
+++ b/chrome/browser/ui/cocoa/dock_icon.mm
@@ -34,6 +34,28 @@ constexpr struct {
     {1, 3, 0.2},
 };
 
+// Tint the current app icon without changing its alpha mask, so product
+// variants retain custom icons and the download badge remains untinted.
+NSImage* AppIconWithVariantTint(NSImage* appIcon, NSSize size, NSColor* tint) {
+  NSImage* tintedIcon = [[NSImage alloc] initWithSize:size];
+  const NSRect iconRect = NSMakeRect(0, 0, size.width, size.height);
+
+  [tintedIcon lockFocus];
+  [appIcon drawInRect:iconRect
+             fromRect:NSZeroRect
+            operation:NSCompositingOperationSourceOver
+             fraction:1.0];
+  [tint setFill];
+  NSRectFillUsingOperation(iconRect, NSCompositingOperationColor);
+  [appIcon drawInRect:iconRect
+             fromRect:NSZeroRect
+            operation:NSCompositingOperationDestinationIn
+             fraction:1.0];
+  [tintedIcon unlockFocus];
+
+  return tintedIcon;
+}
+
 }  // namespace
 
 // A view that draws our dock tile.
@@ -49,6 +71,8 @@ constexpr struct {
 // Indicates the amount of progress made of the download. Ranges from [0..1].
 @property(nonatomic) float progress;
 
+@property(nonatomic, strong) NSColor* variantColor;
+
 @end
 
 @implementation DockTileView
@@ -56,6 +80,7 @@ constexpr struct {
 @synthesize downloads = _downloads;
 @synthesize indeterminate = _indeterminate;
 @synthesize progress = _progress;
+@synthesize variantColor = _variantColor;
 
 - (void)drawRect:(NSRect)dirtyRect {
   // This needs to draw the current app icon, whether it's using the default
@@ -74,6 +99,9 @@ constexpr struct {
   // Therefore, use [NSImage imageNamed:NSImageNameApplicationIcon].
 
   NSImage* appIcon = [NSImage imageNamed:NSImageNameApplicationIcon];
+  if (_variantColor) {
+    appIcon = AppIconWithVariantTint(appIcon, self.bounds.size, _variantColor);
+  }
   [appIcon drawInRect:self.bounds
              fromRect:NSZeroRect
             operation:NSCompositingOperationSourceOver
@@ -245,6 +273,19 @@ constexpr struct {
   [NSApp.dockTile display];
 }
 
+- (void)setDockIconVariantColor:(NSColor*)color {
+  DCHECK_CURRENTLY_ON(BrowserThread::UI);
+  DockTileView* dockTileView =
+      base::apple::ObjCCast<DockTileView>(NSApp.dockTile.contentView);
+
+  BOOL sameColor = color == [dockTileView variantColor] ||
+                   [color isEqual:[dockTileView variantColor]];
+  if (!sameColor) {
+    [dockTileView setVariantColor:color];
+    _forceUpdate = YES;
+  }
+}
+
 - (void)setDownloads:(int)downloads {
   CHECK_CURRENTLY_ON(BrowserThread::UI, base::NotFatalUntil::M162);
   DockTileView* dockTileView =
