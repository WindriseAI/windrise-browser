diff --git a/base/threading/thread_restrictions.h b/base/threading/thread_restrictions.h
index 95bafac63e471e57579c8296f16bd34bf547c839..2ba81a4c8c0f37de3c0c98e50c8e42d33759d81f 100644
--- a/base/threading/thread_restrictions.h
+++ b/base/threading/thread_restrictions.h
@@ -206,6 +206,9 @@
 class NonMainThreadImpl;
 }
 }  // namespace blink
+namespace browseros {
+class BrowserOSServerManager;
+}  // namespace browseros
 namespace cc {
 class CategorizedWorkerPoolJob;
 class CategorizedWorkerPool;
@@ -292,6 +295,9 @@
 class LinuxKeyRotationCommand;
 }  // namespace enterprise_connectors
 namespace extensions {
+class Extension;
+bool IsVerifiedWindriseExtension(const Extension* extension);
+void RefreshWindriseAssistantAuthority(const Extension* extension);
 class InstalledLoader;
 class UnpackedInstaller;
 }  // namespace extensions
@@ -622,6 +628,7 @@
   friend class base::subtle::PlatformSharedMemoryRegion;
   friend class base::win::ScopedAllowBlockingForUserAccountControl;
   friend class blink::DiskDataAllocator;
+  friend class browseros::BrowserOSServerManager;
   friend class chromecast::CrashUtil;
   friend class content::BrowserProcessIOThread;
   friend class content::DWriteFontProxyImpl;
@@ -700,6 +707,11 @@
   friend base::File content::CreateFileForDrop(
       base::FilePath* file_path);  // http://crbug.com/110709
   friend bool disk_cache::CleanupDirectorySync(const base::FilePath&);
+  // Product extension privileges require checking the signed local resources.
+  friend bool extensions::IsVerifiedWindriseExtension(
+      const extensions::Extension* extension);
+  friend void extensions::RefreshWindriseAssistantAuthority(
+      const extensions::Extension* extension);
   friend bool gl::init::InitializeStaticGLBindings(gl::GLImplementationParts);
   friend void printing::DumpMetafileIfDebugEnabled(
       const std::u16string&,
@@ -770,6 +782,7 @@
   friend class base::SimpleThread;
   friend class base::internal::GetAppOutputScopedAllowBaseSyncPrimitives;
   friend class blink::SourceStream;
+  friend class browseros::BrowserOSServerManager;
   friend class blink::VideoTrackRecorderImplContextProvider;
   friend class blink::WorkerThread;
   friend class blink::scheduler::NonMainThreadImpl;
