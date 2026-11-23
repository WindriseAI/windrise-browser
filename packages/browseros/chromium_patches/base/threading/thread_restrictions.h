diff --git a/base/threading/thread_restrictions.h b/base/threading/thread_restrictions.h
index 95bafac63e..2ba81a4c8c 100644
--- a/base/threading/thread_restrictions.h
+++ b/base/threading/thread_restrictions.h
@@ -206,6 +206,13 @@ namespace scheduler {
 class NonMainThreadImpl;
 }
 }  // namespace blink
+namespace browseros {
+class BrowserOSExtensionLoader;
+class BrowserOSServerManager;
+namespace onboarding {
+bool HasDesktopOnboardingMarker(const base::FilePath& user_data_dir);
+}  // namespace onboarding
+}  // namespace browseros
 namespace cc {
 class CategorizedWorkerPoolJob;
 class CategorizedWorkerPool;
@@ -292,6 +299,9 @@ namespace enterprise_connectors {
 class LinuxKeyRotationCommand;
 }  // namespace enterprise_connectors
 namespace extensions {
+class Extension;
+bool IsVerifiedWindriseExtension(const Extension* extension);
+void RefreshWindriseAssistantAuthority(const Extension* extension);
 class InstalledLoader;
 class UnpackedInstaller;
 }  // namespace extensions
@@ -622,6 +632,8 @@ class BASE_EXPORT ScopedAllowBlocking {
   friend class base::subtle::PlatformSharedMemoryRegion;
   friend class base::win::ScopedAllowBlockingForUserAccountControl;
   friend class blink::DiskDataAllocator;
+  friend class browseros::BrowserOSExtensionLoader;
+  friend class browseros::BrowserOSServerManager;
   friend class chromecast::CrashUtil;
   friend class content::BrowserProcessIOThread;
   friend class content::DWriteFontProxyImpl;
@@ -700,6 +712,14 @@ class BASE_EXPORT ScopedAllowBlocking {
   friend base::File content::CreateFileForDrop(
       base::FilePath* file_path);  // http://crbug.com/110709
   friend bool disk_cache::CleanupDirectorySync(const base::FilePath&);
+  // Checking the desktop-written User Data onboarding marker on first launch.
+  friend bool browseros::onboarding::HasDesktopOnboardingMarker(
+      const base::FilePath& user_data_dir);
+  // Product extension privileges require checking the signed local resources.
+  friend bool extensions::IsVerifiedWindriseExtension(
+      const extensions::Extension* extension);
+  friend void extensions::RefreshWindriseAssistantAuthority(
+      const extensions::Extension* extension);
   friend bool gl::init::InitializeStaticGLBindings(gl::GLImplementationParts);
   friend void printing::DumpMetafileIfDebugEnabled(
       const std::u16string&,
@@ -770,6 +790,7 @@ class BASE_EXPORT ScopedAllowBaseSyncPrimitives {
   friend class base::SimpleThread;
   friend class base::internal::GetAppOutputScopedAllowBaseSyncPrimitives;
   friend class blink::SourceStream;
+  friend class browseros::BrowserOSServerManager;
   friend class blink::VideoTrackRecorderImplContextProvider;
   friend class blink::WorkerThread;
   friend class blink::scheduler::NonMainThreadImpl;
