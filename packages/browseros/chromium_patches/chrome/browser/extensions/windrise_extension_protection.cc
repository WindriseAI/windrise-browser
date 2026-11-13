diff --git a/chrome/browser/extensions/windrise_extension_protection.cc b/chrome/browser/extensions/windrise_extension_protection.cc
new file mode 100644
index 0000000000000000000000000000000000000000..622c8730b398d9113e264b1331b382670909bf18
--- /dev/null
+++ b/chrome/browser/extensions/windrise_extension_protection.cc
@@ -0,0 +1,341 @@
+// Copyright 2026 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+#include "chrome/browser/extensions/windrise_extension_protection.h"
+
+#include <algorithm>
+#include <array>
+#include <map>
+#include <memory>
+#include <optional>
+#include <string>
+#include <string_view>
+#include <vector>
+
+#include "base/base64.h"
+#include "base/callback_list.h"
+#include "base/command_line.h"
+#include "base/containers/span.h"
+#include "base/files/file_enumerator.h"
+#include "base/files/file_util.h"
+#include "base/files/file_path_watcher.h"
+#include "base/functional/bind.h"
+#include "base/no_destructor.h"
+#include "base/strings/string_number_conversions.h"
+#include "base/strings/string_util.h"
+#include "base/threading/thread_restrictions.h"
+#include "base/time/time.h"
+#include "base/timer/timer.h"
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
+bool VerifySignedDirectory(const base::FilePath& root, std::string* digest) {
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
+  if (inventory != expected) {
+    return false;
+  }
+  *digest = base::HexEncodeLower(crypto::hash::Sha256(inventory));
+  return true;
+}
+
+base::RepeatingClosureList& AuthorityObservers() {
+  static base::NoDestructor<base::RepeatingClosureList> observers;
+  return *observers;
+}
+
+const std::optional<crypto::keypair::PublicKey>& LeaseKey() {
+  static const base::NoDestructor<std::optional<crypto::keypair::PublicKey>> key(
+      []() -> std::optional<crypto::keypair::PublicKey> {
+        std::string der;
+        if (!base::Base64Decode(
+                base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
+                    "windrise-assistant-lease-public-key"), &der)) {
+          return std::nullopt;
+        }
+        auto parsed = crypto::keypair::PublicKey::FromSubjectPublicKeyInfo(
+            base::as_byte_span(der));
+        if (!parsed || !parsed->IsRsa()) {
+          return std::nullopt;
+        }
+        return parsed;
+      }());
+  return *key;
+}
+
+struct Verification {
+  scoped_refptr<const Extension> extension;
+  bool valid = false;
+  bool authorized = false;
+  std::string digest;
+  std::string last_lease;
+  base::FilePath lease_path;
+  std::string lease_buffer;
+  std::string signature_buffer;
+  base::TimeTicks lease_deadline;
+  base::RepeatingTimer authority_timer;
+  base::FilePathWatcher watcher;
+};
+
+using Verifications =
+    std::map<base::FilePath, std::unique_ptr<Verification>>;
+Verifications& VerifiedExtensions() {
+  static base::NoDestructor<Verifications> records;
+  return *records;
+}
+
+bool HasLiveLease(Verification* record) {
+  std::string& lease = record->lease_buffer;
+  if (!record->valid || !LeaseKey() ||
+      !base::ReadFileToStringWithMaxSize(record->lease_path, &lease, 2048)) {
+    return false;
+  }
+  const auto ticks = base::TimeTicks::Now();
+  if (lease == record->last_lease) {
+    return ticks < record->lease_deadline;
+  }
+  std::array<std::string_view, 6> lines;
+  std::string_view remaining(lease);
+  for (size_t i = 0; i < 5; ++i) {
+    const size_t end = remaining.find('\n');
+    if (end == std::string_view::npos) {
+      return false;
+    }
+    lines[i] = remaining.substr(0, end);
+    remaining.remove_prefix(end + 1);
+  }
+  lines[5] = remaining;
+  int64_t expires = 0;
+  const int64_t now = base::Time::Now().InMillisecondsSinceUnixEpoch();
+  if (lines[0] != "windrise-assistant-lease-v1" ||
+      lines[1] != kAssistantId || lines[2] != record->digest ||
+      !lines[5].empty() || !base::StringToInt64(lines[3], &expires) ||
+      expires <= now || expires - now > 8000) {
+    return false;
+  }
+  std::string& signature = record->signature_buffer;
+  if (!base::Base64Decode(lines[4], &signature)) {
+    return false;
+  }
+  const std::string_view payload(lease.data(), lines[4].data() - lease.data());
+  if (!crypto::sign::Verify(crypto::sign::RSA_PKCS1_SHA256, *LeaseKey(),
+                            base::as_byte_span(payload),
+                            base::as_byte_span(signature))) {
+    return false;
+  }
+  record->last_lease.swap(lease);
+  record->lease_deadline = ticks + base::Milliseconds(expires - now);
+  return true;
+}
+
+void PollAuthority(Verification* record) {
+  bool authorized = HasLiveLease(record);
+  if (authorized && !record->authorized) {
+    // Rebuilding a previously revoked layer requires every signed byte again.
+    std::string digest;
+    if (!VerifySignedDirectory(record->extension->path(), &digest) ||
+        digest != record->digest) {
+      record->valid = false;
+      authorized = false;
+    }
+  }
+  if (record->authorized != authorized) {
+    record->authorized = authorized;
+    AuthorityObservers().Notify();
+  }
+}
+}  // namespace
+
+void RefreshWindriseAssistantAuthority(const Extension* extension) {
+  const auto found = VerifiedExtensions().find(extension->path());
+  if (found != VerifiedExtensions().end() &&
+      found->second->extension.get() == extension) {
+    base::ScopedAllowBlocking allow_blocking;
+    PollAuthority(found->second.get());
+  }
+}
+
+bool IsVerifiedWindriseExtension(const Extension* extension) {
+  if (!extension || extension->id() != kAssistantId ||
+      extension->public_key() != kPublicKey ||
+      extension->location() != mojom::ManifestLocation::kCommandLine) {
+    return false;
+  }
+  auto& record = VerifiedExtensions()[extension->path()];
+  if (record && record->extension.get() == extension) {
+    return record->valid;
+  }
+  base::ScopedAllowBlocking allow_blocking;
+  auto next = std::make_unique<Verification>();
+  next->extension = extension;
+  next->lease_path = extension->path().DirName().AppendASCII(
+      ".windrise-assistant-lease");
+  // Start monitoring before reading any bytes. Any write, deletion, rename or
+  // watcher failure revokes this Extension object's trust until it is reloaded.
+  // Restoring timestamps cannot restore trust. Never rehash on activity pulses.
+  if (base::FilePathWatcher::RecursiveWatchAvailable() &&
+      next->watcher.Watch(
+          extension->path(), base::FilePathWatcher::Type::kRecursive,
+          base::BindRepeating(
+              [](Verification* result, const base::FilePath&, bool) {
+                result->valid = false;
+                result->authorized = false;
+                result->authority_timer.Stop();
+                AuthorityObservers().Notify();
+              },
+              base::Unretained(next.get())))) {
+    next->valid = VerifySignedDirectory(extension->path(), &next->digest);
+  }
+  const bool valid = next->valid;
+  record = std::move(next);
+  if (valid) {
+    record->authority_timer.Start(
+        FROM_HERE, base::Milliseconds(250),
+        base::BindRepeating(&RefreshWindriseAssistantAuthority,
+                            base::Unretained(extension)));
+  }
+  return valid;
+}
+
+bool IsAuthorizedWindriseExtension(const Extension* extension) {
+  if (!IsVerifiedWindriseExtension(extension)) {
+    return false;
+  }
+  const auto& record = VerifiedExtensions().at(extension->path());
+  if (!record->authorized) {
+    RefreshWindriseAssistantAuthority(extension);
+  }
+  return record->authorized &&
+         base::TimeTicks::Now() < record->lease_deadline;
+}
+
+base::CallbackListSubscription AddWindriseAssistantAuthorityObserver(
+    base::RepeatingClosure callback) {
+  return AuthorityObservers().Add(std::move(callback));
+}
+
+void RegisterWindriseExtensionProtection(ManagementPolicy* policy) {
+  static base::NoDestructor<WindriseProtection> provider;
+  policy->RegisterProvider(provider.get());
+}
+}  // namespace extensions
