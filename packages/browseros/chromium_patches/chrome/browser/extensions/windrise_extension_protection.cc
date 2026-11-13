diff --git a/chrome/browser/extensions/windrise_extension_protection.cc b/chrome/browser/extensions/windrise_extension_protection.cc
new file mode 100644
index 000000000..97a38fb07
--- /dev/null
+++ b/chrome/browser/extensions/windrise_extension_protection.cc
@@ -0,0 +1,195 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+#include "chrome/browser/extensions/windrise_extension_protection.h"
+
+#include <algorithm>
+#include <map>
+#include <memory>
+#include <string>
+#include <vector>
+
+#include "base/base64.h"
+#include "base/containers/span.h"
+#include "base/files/file_enumerator.h"
+#include "base/files/file_util.h"
+#include "base/files/file_path_watcher.h"
+#include "base/functional/bind.h"
+#include "base/no_destructor.h"
+#include "base/strings/string_number_conversions.h"
+#include "base/strings/string_util.h"
+#include "base/threading/thread_restrictions.h"
+#include "crypto/hash.h"
+#include "crypto/keypair.h"
+#include "crypto/sign.h"
+#include "extensions/browser/management_policy.h"
+#include "extensions/common/extension.h"
+#include "extensions/common/mojom/manifest.mojom.h"
+
+namespace extensions {
+namespace {
+constexpr char kAssistantId[] = "amlnokjigkbgngdgkofikfphjbknfodp";
+constexpr char kPublicKey[] =
+    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu/BdI9vbyMwLGJs9imDubXmoawZD3wzoYTUBSIru4A4+KflzIz04Ps8JUFpUmSJdUkgedCujSxQFt+ktysIc6tZAo7xXDqW/wDHr3py5sX7rfAILXh8o8lG9fRbJZjBj38rb/PTMvmvRJMCKnmM9MAaPwt3DT2Nj8xzR08GzeWdf6mfThswAA5w0YRIlVMhExKBFfB3b6Nb969QPha5//pxZMBoPSXktb4Gemx3esOtVWoWpxFEbfYs2td1MXGMlvziBN96hZ+49TcnFRKdzJoALB0q9xp1DHPlM0n9EMNKY+Fps0/17GrwGCYhb3DSyWEX7MXQLZraiT2eWQxpiDwIDAQAB";
+constexpr char kInventory[] = ".windrise-integrity";
+constexpr char kSignature[] = ".windrise-signature";
+
+bool CollectFiles(const base::FilePath& root,
+                  const base::FilePath& directory,
+                  std::vector<std::string>* names) {
+  if (base::IsLink(directory)) {
+    return false;
+  }
+  base::FileEnumerator entries(
+      directory, false,
+      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES,
+      base::FilePath::StringType(), base::FileEnumerator::FolderSearchPolicy::ALL,
+      base::FileEnumerator::ErrorPolicy::STOP_ENUMERATION);
+  for (base::FilePath entry = entries.Next(); !entry.empty();
+       entry = entries.Next()) {
+    if (base::IsLink(entry)) {
+      return false;
+    }
+    if (entries.GetInfo().IsDirectory()) {
+      if (!CollectFiles(root, entry, names)) {
+        return false;
+      }
+      continue;
+    }
+    base::FilePath relative;
+    if (!root.AppendRelativePath(entry, &relative)) {
+      return false;
+    }
+    std::string name = relative.AsUTF8Unsafe();
+    std::replace(name.begin(), name.end(), '\\', '/');
+    if (name == kInventory || name == kSignature) {
+      continue;
+    }
+    if (name.find_first_not_of(
+            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-/") !=
+        std::string::npos) {
+      return false;
+    }
+    names->push_back(std::move(name));
+  }
+  return entries.GetError() == base::File::FILE_OK;
+}
+
+class WindriseProtection : public ManagementPolicy::Provider {
+ public:
+  std::string GetDebugPolicyProviderName() const override {
+    return "Windrise assistant protection";
+  }
+  bool UserMayLoad(const Extension* extension,
+                   std::u16string* error) const override {
+    return extension->id() != kAssistantId ||
+           IsVerifiedWindriseExtension(extension);
+  }
+  bool UserMayModifySettings(const Extension* extension,
+                             std::u16string* error) const override {
+    return !IsVerifiedWindriseExtension(extension);
+  }
+  bool ExtensionMayModifySettings(const Extension* source,
+                                  const Extension* extension,
+                                  std::u16string* error) const override {
+    return !IsVerifiedWindriseExtension(extension);
+  }
+  bool MustRemainEnabled(const Extension* extension,
+                         std::u16string* error) const override {
+    return IsVerifiedWindriseExtension(extension);
+  }
+  bool MustRemainInstalled(const Extension* extension,
+                           std::u16string* error) const override {
+    return IsVerifiedWindriseExtension(extension);
+  }
+};
+
+bool VerifySignedDirectory(const base::FilePath& root) {
+  std::string inventory;
+  std::string signature_text;
+  std::string signature;
+  std::string public_der;
+  if (!base::ReadFileToStringWithMaxSize(root.AppendASCII(kInventory), &inventory,
+                                        1024 * 1024) ||
+      !base::ReadFileToStringWithMaxSize(root.AppendASCII(kSignature),
+                                        &signature_text, 1024) ||
+      !base::Base64Decode(base::TrimWhitespaceASCII(signature_text, base::TRIM_ALL),
+                          &signature) ||
+      !base::Base64Decode(kPublicKey, &public_der)) {
+    return false;
+  }
+  const auto key = crypto::keypair::PublicKey::FromSubjectPublicKeyInfo(
+      base::as_byte_span(public_der));
+  if (!key || !key->IsRsa() ||
+      !crypto::sign::Verify(crypto::sign::RSA_PKCS1_SHA256, *key,
+                            base::as_byte_span(inventory),
+                            base::as_byte_span(signature))) {
+    return false;
+  }
+  std::vector<std::string> names;
+  if (!CollectFiles(root, root, &names)) {
+    return false;
+  }
+  std::sort(names.begin(), names.end());
+  std::string expected = "windrise-extension-v1\n";
+  expected.append(kAssistantId).push_back('\n');
+  std::string contents;
+  for (const std::string& name : names) {
+    if (!base::ReadFileToStringWithMaxSize(root.AppendASCII(name), &contents,
+                                          64 * 1024 * 1024)) {
+      return false;
+    }
+    const auto hash = crypto::hash::Sha256(contents);
+    expected.append(base::HexEncodeLower(hash))
+        .append(" ")
+        .append(name)
+        .push_back('\n');
+  }
+  return inventory == expected;
+}
+
+struct Verification {
+  scoped_refptr<const Extension> extension;
+  bool valid = false;
+  base::FilePathWatcher watcher;
+};
+}  // namespace
+
+bool IsVerifiedWindriseExtension(const Extension* extension) {
+  if (!extension || extension->id() != kAssistantId ||
+      extension->public_key() != kPublicKey ||
+      extension->location() != mojom::ManifestLocation::kCommandLine) {
+    return false;
+  }
+  static base::NoDestructor<
+      std::map<base::FilePath, std::unique_ptr<Verification>>> verified;
+  auto& record = (*verified)[extension->path()];
+  if (record && record->extension.get() == extension) {
+    return record->valid;
+  }
+  base::ScopedAllowBlocking allow_blocking;
+  auto next = std::make_unique<Verification>();
+  next->extension = extension;
+  // Start monitoring before reading any bytes. Any write, deletion, rename or
+  // watcher failure revokes this Extension object's trust until it is reloaded.
+  // Restoring timestamps cannot restore trust. Never rehash on activity pulses.
+  if (base::FilePathWatcher::RecursiveWatchAvailable() &&
+      next->watcher.Watch(
+          extension->path(), base::FilePathWatcher::Type::kRecursive,
+          base::BindRepeating(
+              [](Verification* result, const base::FilePath&, bool) {
+                result->valid = false;
+              },
+              base::Unretained(next.get())))) {
+    next->valid = VerifySignedDirectory(extension->path());
+  }
+  const bool valid = next->valid;
+  record = std::move(next);
+  return valid;
+}
+
+void RegisterWindriseExtensionProtection(ManagementPolicy* policy) {
+  static base::NoDestructor<WindriseProtection> provider;
+  policy->RegisterProvider(provider.get());
+}
+}  // namespace extensions
