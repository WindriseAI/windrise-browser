diff --git a/chrome/browser/browseros/extensions/browseros_extension_loader.cc b/chrome/browser/browseros/extensions/browseros_extension_loader.cc
new file mode 100644
index 0000000000000000000000000000000000000000..9b14254353cf16a96d5866048dfff8606c8d3a4f
--- /dev/null
+++ b/chrome/browser/browseros/extensions/browseros_extension_loader.cc
@@ -0,0 +1,642 @@
+// Copyright 2024 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/browseros/extensions/browseros_extension_loader.h"
+
+#include <optional>
+#include <utility>
+
+#include "base/command_line.h"
+#include "base/files/file_util.h"
+#include "base/functional/bind.h"
+#include "base/json/values_util.h"
+#include "base/logging.h"
+#include "base/strings/string_tokenizer.h"
+#include "base/supports_user_data.h"
+#include "base/task/single_thread_task_runner.h"
+#include "base/threading/thread_restrictions.h"
+#include "base/version.h"
+#include "chrome/browser/browseros/core/browseros_constants.h"
+#include "chrome/browser/browseros/core/browseros_prefs.h"
+#include "chrome/browser/browseros/core/browseros_switches.h"
+#include "chrome/browser/extensions/external_provider_impl.h"
+#include "chrome/browser/profiles/profile.h"
+#include "components/prefs/scoped_user_pref_update.h"
+#include "content/public/browser/browser_thread.h"
+#include "extensions/browser/delayed_install_manager.h"
+#include "extensions/browser/disable_reason.h"
+#include "extensions/browser/extension_prefs.h"
+#include "extensions/browser/extension_registrar.h"
+#include "extensions/browser/extensions_browser_client.h"
+#include "extensions/browser/unpacked_installer.h"
+#include "extensions/common/extension.h"
+#include "extensions/common/switches.h"
+
+namespace browseros {
+namespace {
+
+constexpr char kLoaderReferenceKey[] = "browseros.extension_loader";
+constexpr base::TimeDelta kReadinessTimeout = base::Seconds(45);
+constexpr base::TimeDelta kMaintenanceInterval = base::Minutes(15);
+constexpr base::TimeDelta kInstallGracePeriod = base::Minutes(30);
+constexpr char kGraceVersion[] = "version";
+constexpr char kGraceDeadline[] = "deadline";
+constexpr char kGraceReleased[] = "released";
+
+// The external provider retains ownership. Profile data is only a weak lookup
+// for native callers, and never extends a provider or profile's lifetime.
+struct LoaderReference : public base::SupportsUserData::Data {
+  explicit LoaderReference(base::WeakPtr<BrowserOSExtensionLoader> loader)
+      : loader(std::move(loader)) {}
+  base::WeakPtr<BrowserOSExtensionLoader> loader;
+};
+
+void PostResult(BrowserOSExtensionLoader::ReadyCallback callback, bool ready) {
+  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
+      FROM_HERE, base::BindOnce(std::move(callback), ready));
+}
+
+// Returns true when --load-extension already supplies a signed Windrise
+// assistant directory (e.g. launched by the desktop app with an updated copy),
+// so the bundled fallback does not race or downgrade it.
+bool HasCommandLineWindriseAssistant(const base::CommandLine& command_line) {
+  if (!command_line.HasSwitch(extensions::switches::kLoadExtension)) {
+    return false;
+  }
+  const base::CommandLine::StringType path_list =
+      command_line.GetSwitchValueNative(extensions::switches::kLoadExtension);
+  base::StringTokenizerT<base::CommandLine::StringType,
+                         base::CommandLine::StringType::const_iterator>
+      tokenizer(path_list, FILE_PATH_LITERAL(","));
+  while (tokenizer.GetNext()) {
+    const base::FilePath dir(tokenizer.token_piece());
+    if (base::PathExists(dir.AppendASCII(".windrise-integrity")) &&
+        base::PathExists(dir.AppendASCII(".windrise-signature"))) {
+      return true;
+    }
+  }
+  return false;
+}
+
+}  // namespace
+
+BrowserOSExtensionLoader::BrowserOSExtensionLoader(Profile* profile)
+    : profile_(profile),
+      // Populated when a bundled signed Windrise assistant is present (or
+      // already loaded via --load-extension); stays empty when absent so
+      // onboarding never blocks on builds without the bundled assistant.
+      primary_id_(""),
+      installer_(std::make_unique<BrowserOSExtensionInstaller>()),
+      maintainer_(std::make_unique<BrowserOSExtensionMaintainer>(profile)) {
+  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
+  profile_->SetUserData(
+      kLoaderReferenceKey,
+      std::make_unique<LoaderReference>(weak_ptr_factory_.GetWeakPtr()));
+  profile_observation_.Observe(profile_);
+  // Observe before InstalledLoader runs: READY may precede our asynchronous
+  // bundle discovery, and secondary bundles can become ready after the primary.
+  registry_observation_.Observe(extensions::ExtensionRegistry::Get(profile_));
+  if (extensions::ExtensionRegistry::Get(profile_)->GetExtensionById(
+          kWindriseAssistantExtensionId,
+          extensions::ExtensionRegistry::EVERYTHING)) {
+    primary_id_ = kWindriseAssistantExtensionId;
+  }
+  InitializeInstallGrace();
+  ScheduleInstallGraceExpiry();
+}
+
+BrowserOSExtensionLoader::~BrowserOSExtensionLoader() {
+  Shutdown();
+}
+
+// static
+bool BrowserOSExtensionLoader::LoadBundledWindriseAssistantIfNeeded(
+    Profile* profile) {
+  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
+  if (!profile || profile->IsOffTheRecord() || profile->IsGuestSession() ||
+      profile->IsSystemProfile()) {
+    return false;
+  }
+  const base::CommandLine* command_line =
+      base::CommandLine::ForCurrentProcess();
+  if (command_line->HasSwitch(kDisableExtensions) ||
+      command_line->HasSwitch(kLegacyDisableExtensions)) {
+    return false;
+  }
+  auto* registry = extensions::ExtensionRegistry::Get(profile);
+  if (registry &&
+      registry->GetExtensionById(kWindriseAssistantExtensionId,
+                                 extensions::ExtensionRegistry::EVERYTHING)) {
+    return true;
+  }
+  base::FilePath bundled_dir;
+  {
+    base::ScopedAllowBlocking allow_blocking;
+    if (HasCommandLineWindriseAssistant(*command_line)) {
+      return true;
+    }
+    bundled_dir =
+        BrowserOSExtensionInstaller::ResolveBundledWindriseAssistantDir();
+  }
+  if (bundled_dir.empty()) {
+    return false;
+  }
+  std::string loaded_id;
+  const bool started =
+      extensions::UnpackedInstaller::Create(profile)->LoadFromCommandLine(
+          bundled_dir, &loaded_id, /*only_allow_apps=*/false);
+  if (!started || loaded_id != kWindriseAssistantExtensionId) {
+    LOG(WARNING) << "windrise: Bundled signed assistant failed to load from "
+                 << bundled_dir.value();
+    return false;
+  }
+  LOG(INFO) << "windrise: Loaded bundled signed assistant from "
+            << bundled_dir.value();
+  return true;
+}
+
+// static
+void BrowserOSExtensionLoader::EnsurePrimaryExtensionReady(
+    Profile* profile,
+    ReadyCallback callback) {
+  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
+  auto* reference = static_cast<LoaderReference*>(
+      profile->GetOriginalProfile()->GetUserData(kLoaderReferenceKey));
+  if (!reference || !reference->loader) {
+    PostResult(std::move(callback), false);
+    return;
+  }
+  reference->loader->EnsureReady(std::move(callback));
+}
+
+// static
+BrowserOSExtensionLoader::UpdateInstallPolicy
+BrowserOSExtensionLoader::GetUpdateInstallPolicy(Profile* profile,
+                                                 const std::string& id) {
+  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
+  if (!IsActiveBrowserOSExtension(id)) {
+    return UpdateInstallPolicy::kNormal;
+  }
+  profile = profile->GetOriginalProfile();
+  const auto* record =
+      profile->GetPrefs()->GetDict(prefs::kExtensionInstallGrace).FindDict(id);
+  const auto* installed =
+      extensions::ExtensionRegistry::Get(profile)->enabled_extensions().GetByID(
+          id);
+  if (!record || !installed) {
+    // Missing, disabled/corrupt, or terminated: allow recovery.
+    return UpdateInstallPolicy::kNormal;
+  }
+  const auto* version = record->FindString(kGraceVersion);
+  const auto deadline = base::ValueToTime(record->Find(kGraceDeadline));
+  if (!version || *version != installed->version().GetString() || !deadline) {
+    return UpdateInstallPolicy::kNormal;
+  }
+  // Release is one-way even if the wall clock is subsequently moved backwards.
+  return record->FindBool(kGraceReleased).value_or(false) ||
+                 *deadline <= base::Time::Now()
+             ? UpdateInstallPolicy::kInstallImmediately
+             : UpdateInstallPolicy::kDefer;
+}
+
+void BrowserOSExtensionLoader::InitializeInstallGrace() {
+  auto* pref_service = profile_->GetPrefs();
+  if (pref_service->GetUserPrefValue(prefs::kExtensionInstallGrace)) {
+    return;
+  }
+  base::DictValue records;
+  // Absence of a new pref is not proof of a new user. Freeze eligibility once,
+  // before discovery/install, so a restart cannot enroll an existing profile
+  // or restart a deadline. An already installed ID is never a fresh bundle.
+  if (profile_->IsNewProfile() &&
+      !pref_service->GetBoolean(prefs::kOnboardingCompleted)) {
+    for (const auto& id : GetActiveBrowserOSExtensionIds()) {
+      if (!extensions::ExtensionPrefs::Get(profile_)->GetInstalledExtensionInfo(
+              id)) {
+        records.Set(id, base::DictValue());
+      }
+    }
+  }
+  pref_service->SetDict(prefs::kExtensionInstallGrace, std::move(records));
+}
+
+void BrowserOSExtensionLoader::RecordBundledVersions() {
+  {
+    ScopedDictPrefUpdate update(profile_->GetPrefs(),
+                                prefs::kExtensionInstallGrace);
+    for (const auto& id : GetActiveBrowserOSExtensionIds()) {
+      auto* record = update->FindDict(id);
+      if (!record || record->contains(kGraceDeadline)) {
+        continue;
+      }
+      const auto* local = bundled_prefs_.FindDict(id);
+      if (!local || failed_bundles_.contains(id)) {
+        update->Remove(id);  // Missing or failed bundles cannot start grace.
+      } else {
+        record->Set(kGraceVersion,
+                    *local->FindString(
+                        extensions::ExternalProviderImpl::kExternalVersion));
+      }
+    }
+  }
+  for (const auto& id : GetActiveBrowserOSExtensionIds()) {
+    StartInstallGraceIfReady(id);
+  }
+}
+
+void BrowserOSExtensionLoader::StartInstallGraceIfReady(const std::string& id) {
+  const auto* extension =
+      extensions::ExtensionRegistry::Get(profile_)->ready_extensions().GetByID(
+          id);
+  if (!extension) {
+    return;
+  }
+  {
+    ScopedDictPrefUpdate update(profile_->GetPrefs(),
+                                prefs::kExtensionInstallGrace);
+    auto* record = update->FindDict(id);
+    if (!record) {
+      return;
+    }
+    const auto* version = record->FindString(kGraceVersion);
+    if (!version) {
+      return;  // Discovery has not supplied the bundle identity yet.
+    }
+    if (*version != extension->version().GetString()) {
+      update->Remove(id);
+      return;
+    }
+    if (record->contains(kGraceDeadline)) {
+      return;
+    }
+    record->Set(kGraceDeadline,
+                base::TimeToValue(base::Time::Now() + kInstallGracePeriod));
+    LOG(INFO) << "windrise: Started 30-minute bundled extension grace for "
+              << id;
+  }
+  ScheduleInstallGraceExpiry();
+}
+
+void BrowserOSExtensionLoader::ScheduleInstallGraceExpiry() {
+  base::Time next;
+  for (const auto& id : GetActiveBrowserOSExtensionIds()) {
+    const auto* record = profile_->GetPrefs()
+                             ->GetDict(prefs::kExtensionInstallGrace)
+                             .FindDict(id);
+    const auto deadline =
+        record ? base::ValueToTime(record->Find(kGraceDeadline)) : std::nullopt;
+    if (deadline && !record->FindBool(kGraceReleased).value_or(false) &&
+        (next.is_null() || *deadline < next)) {
+      next = *deadline;
+    }
+  }
+  install_grace_timer_.Stop();
+  if (!next.is_null()) {
+    install_grace_timer_.Start(
+        FROM_HERE, next, this,
+        &BrowserOSExtensionLoader::OnInstallGraceExpired);
+  }
+}
+
+void BrowserOSExtensionLoader::OnInstallGraceExpired() {
+  std::vector<std::string> ids;
+  {
+    ScopedDictPrefUpdate update(profile_->GetPrefs(),
+                                prefs::kExtensionInstallGrace);
+    for (const auto& id : GetActiveBrowserOSExtensionIds()) {
+      auto* record = update->FindDict(id);
+      const auto deadline =
+          record ? base::ValueToTime(record->Find(kGraceDeadline))
+                 : std::nullopt;
+      if (deadline && *deadline <= base::Time::Now() &&
+          !record->FindBool(kGraceReleased).value_or(false)) {
+        ids.push_back(id);
+        // Keep the release policy until READY reports a different version.
+        // A background installer may still be unpacking and therefore absent
+        // from DelayedInstallManager when this one-time timer fires.
+        record->Set(kGraceReleased, true);
+      }
+    }
+  }
+  // Release previously staged updates through all Chromium gates, even when
+  // pages keep the extension busy. This does not bypass imports or safety
+  // gates. The persisted deadline, not process uptime, decides when activation
+  // resumes.
+  for (const auto& id : ids) {
+    extensions::DelayedInstallManager::Get(profile_)
+        ->FinishDelayedInstallationIfReady(id, /*install_immediately=*/true);
+    LOG(INFO) << "windrise: Bundled extension grace expired for " << id;
+  }
+  ScheduleInstallGraceExpiry();
+}
+
+void BrowserOSExtensionLoader::StartLoading() {
+  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
+  if (!profile_) {
+    return;
+  }
+  if (local_discovery_complete_) {
+    // Each provider visit expects its metadata callback, even after discovery.
+    // Replay the retained snapshot; Chromium deduplicates pending installs.
+    LoadFinished(BuildDesiredPrefs());
+    return;
+  }
+  if (started_) {
+    return;
+  }
+  // Chromium may ask the provider to load repeatedly. Never replace helpers:
+  // their weak replies belong to this single discovery operation.
+  started_ = true;
+  installer_->DiscoverBundled(
+      base::BindOnce(&BrowserOSExtensionLoader::OnLocalDiscoveryComplete,
+                     weak_ptr_factory_.GetWeakPtr()));
+}
+
+void BrowserOSExtensionLoader::EnsureReady(ReadyCallback callback) {
+  if (!profile_) {
+    PostResult(std::move(callback), false);
+    return;
+  }
+  if (local_discovery_complete_ && IsPrimaryReady()) {
+    PostResult(std::move(callback), true);
+    return;
+  }
+  ready_callbacks_.push_back(std::move(callback));
+  if (waiting_for_ready_) {
+    return;
+  }
+  // An explicit retry republishes the bundled CRX, never a remote URL.
+  failed_bundles_.erase(primary_id_);
+  BeginReadiness();
+  if (!started_) {
+    StartLoading();
+  }
+  if (local_discovery_complete_) {
+    PublishUpdates();
+    AdvanceReadiness();
+  }
+}
+
+void BrowserOSExtensionLoader::OnLocalDiscoveryComplete(
+    BundleDiscoveryResult result) {
+  bundled_crx_base_path_ = std::move(result.bundled_path);
+  bundled_assistant_dir_ = std::move(result.bundled_assistant_dir);
+  bundled_prefs_ = std::move(result.prefs);
+  local_discovery_complete_ = true;
+  if (!bundled_assistant_dir_.empty()) {
+    if (LoadBundledWindriseAssistantIfNeeded(profile_)) {
+      primary_id_ = kWindriseAssistantExtensionId;
+    }
+  }
+  RecordBundledVersions();
+  LOG(INFO) << "windrise: Local extension discovery complete: "
+            << bundled_prefs_.size()
+            << " entries, complete=" << result.complete;
+
+  BeginReadiness();
+  // LoadFinished acknowledges metadata, NOT successful installation. Even an
+  // empty/partial bundle must release Chromium's external-provider barrier.
+  LoadFinished(BuildDesiredPrefs());
+  // Updates can call the updater, which is not alive during ExtensionService
+  // initialization. Cross that startup seam before any recovery/maintenance.
+  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
+      FROM_HERE, base::BindOnce(&BrowserOSExtensionLoader::AdvanceReadiness,
+                                weak_ptr_factory_.GetWeakPtr()));
+}
+
+const base::FilePath BrowserOSExtensionLoader::GetBaseCrxFilePath() {
+  return bundled_crx_base_path_;
+}
+
+bool BrowserOSExtensionLoader::NeedsLocalInstall(const std::string& id) const {
+  const base::DictValue* local = bundled_prefs_.FindDict(id);
+  if (!local || failed_bundles_.contains(id)) {
+    return false;
+  }
+  const auto* installed =
+      extensions::ExtensionRegistry::Get(profile_)->GetInstalledExtension(id);
+  const base::Version version(
+      *local->FindString(extensions::ExternalProviderImpl::kExternalVersion));
+  return !installed || installed->version().CompareTo(version) < 0;
+}
+
+bool BrowserOSExtensionLoader::HasUsableLocalPrimary() const {
+  auto* registry = extensions::ExtensionRegistry::Get(profile_);
+  return (bundled_prefs_.contains(primary_id_) &&
+          !failed_bundles_.contains(primary_id_)) ||
+         (primary_id_ == kWindriseAssistantExtensionId &&
+          !bundled_assistant_dir_.empty()) ||
+         registry->enabled_extensions().Contains(primary_id_) ||
+         registry->terminated_extensions().Contains(primary_id_);
+}
+
+bool BrowserOSExtensionLoader::IsPrimaryReady() const {
+  if (!profile_ || !local_discovery_complete_) {
+    return false;
+  }
+  if (primary_id_.empty()) {
+    return true;
+  }
+  return extensions::ExtensionRegistry::Get(profile_)
+             ->ready_extensions()
+             .Contains(primary_id_) &&
+         !NeedsLocalInstall(primary_id_);
+}
+
+base::DictValue BrowserOSExtensionLoader::BuildDesiredPrefs() const {
+  base::DictValue prefs;
+  auto* registry = extensions::ExtensionRegistry::Get(profile_);
+  for (const std::string& id : GetActiveBrowserOSExtensionIds()) {
+    const auto* local = bundled_prefs_.FindDict(id);
+    if (local && (!failed_bundles_.contains(id) ||
+                  registry->GetInstalledExtension(id))) {
+      prefs.Set(id, local->Clone());
+    }
+  }
+  return prefs;
+}
+
+void BrowserOSExtensionLoader::BeginReadiness() {
+  if (waiting_for_ready_ || !profile_) {
+    return;
+  }
+  waiting_for_ready_ = true;
+  if (!install_observation_.IsObserving()) {
+    // Secondary local installs can finish after the primary is READY. Keep
+    // observing failures so only an explicit retry republishes a bad bundle.
+    install_observation_.Observe(
+        extensions::ExtensionsBrowserClient::Get()->GetInstallTracker(
+            profile_));
+  }
+  if (IsPrimaryReady()) {
+    FinishReadiness(true);
+    return;
+  }
+  // Registry access and registration share the UI sequence. Always inspect
+  // the READY set as well as observing: installed profiles may already be
+  // ready.
+  readiness_timer_.Start(
+      FROM_HERE, kReadinessTimeout,
+      base::BindOnce(&BrowserOSExtensionLoader::OnReadinessTimeout,
+                     weak_ptr_factory_.GetWeakPtr()));
+}
+
+void BrowserOSExtensionLoader::RestorePrimaryExtension() {
+  auto* registry = extensions::ExtensionRegistry::Get(profile_);
+  auto* registrar = extensions::ExtensionRegistrar::Get(profile_);
+  if (registry->terminated_extensions().Contains(primary_id_)) {
+    registrar->ReloadExtension(primary_id_);
+  } else if (registry->disabled_extensions().Contains(primary_id_)) {
+    // Recover user/prompt disablement of a managed product extension, but do
+    // not clear corruption, policy or other safety-related disable reasons.
+    const auto reasons =
+        extensions::ExtensionPrefs::Get(profile_)->GetDisableReasons(
+            primary_id_);
+    for (auto reason : reasons) {
+      if (reason != extensions::disable_reason::DISABLE_USER_ACTION &&
+          reason != extensions::disable_reason::DISABLE_EXTERNAL_EXTENSION &&
+          reason != extensions::disable_reason::DISABLE_PERMISSIONS_INCREASE) {
+        return;
+      }
+    }
+    registrar->EnableExtension(primary_id_);
+  }
+}
+
+void BrowserOSExtensionLoader::AdvanceReadiness() {
+  if (!profile_ || !waiting_for_ready_ || !local_discovery_complete_) {
+    return;
+  }
+  if (IsPrimaryReady()) {
+    FinishReadiness(true);
+    return;
+  }
+  RestorePrimaryExtension();
+  if (!waiting_for_ready_) {
+    return;  // Enabling can synchronously notify READY.
+  }
+  if (IsPrimaryReady()) {
+    FinishReadiness(true);
+    return;
+  }
+  if (!HasUsableLocalPrimary()) {
+    FinishReadiness(false);
+  }
+}
+
+void BrowserOSExtensionLoader::PublishUpdates() {
+  if (profile_ && local_discovery_complete_ && has_owner()) {
+    // Full desired-state snapshots preserve ownership of every active ID.
+    // Chromium deduplicates pending files and refuses same/older versions.
+    OnUpdated(BuildDesiredPrefs());
+  }
+}
+
+void BrowserOSExtensionLoader::FinishReadiness(bool ready) {
+  if (!waiting_for_ready_) {
+    return;
+  }
+  waiting_for_ready_ = false;
+  readiness_timer_.Stop();
+  LOG(INFO) << "windrise: Primary extension readiness "
+            << (ready ? "READY" : "failed; retry available");
+  auto callbacks = std::move(ready_callbacks_);
+  ready_callbacks_.clear();
+  // Post results rather than destroying the onboarding window inside a
+  // registry notification. Weak callbacks at the UI boundary cancel safely.
+  for (auto& callback : callbacks) {
+    PostResult(std::move(callback), ready);
+  }
+  if (ready && !maintenance_started_) {
+    maintenance_started_ = true;
+    maintenance_timer_.Start(
+        FROM_HERE, kMaintenanceInterval, this,
+        &BrowserOSExtensionLoader::MaintainInstalledExtensions);
+    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
+        FROM_HERE,
+        base::BindOnce(&BrowserOSExtensionLoader::MaintainInstalledExtensions,
+                       weak_ptr_factory_.GetWeakPtr()));
+  }
+}
+
+void BrowserOSExtensionLoader::MaintainInstalledExtensions() {
+  if (!profile_ || waiting_for_ready_) {
+    return;
+  }
+  // Retire old upstream extensions and keep local product extensions enabled.
+  maintainer_->MaintainInstalledExtensions();
+}
+
+void BrowserOSExtensionLoader::OnReadinessTimeout() {
+  // This is a UX bound, never evidence that installation succeeded. Chromium
+  // may still finish its own CRX work; Retry joins that work through the
+  // provider.
+  FinishReadiness(false);
+}
+
+void BrowserOSExtensionLoader::OnExtensionReady(
+    content::BrowserContext* context,
+    const extensions::Extension* extension) {
+  StartInstallGraceIfReady(extension->id());
+  if (extension->id() == primary_id_ && IsPrimaryReady()) {
+    FinishReadiness(true);
+  }
+}
+
+void BrowserOSExtensionLoader::OnFinishCrxInstall(
+    content::BrowserContext* context,
+    const base::FilePath& source_file,
+    const std::string& extension_id,
+    const extensions::Extension* extension,
+    bool success) {
+  if (success || !IsActiveBrowserOSExtension(extension_id)) {
+    return;
+  }
+  const auto* local = bundled_prefs_.FindDict(extension_id);
+  if (local &&
+      source_file.AsUTF8Unsafe() ==
+          *local->FindString(extensions::ExternalProviderImpl::kExternalCrx)) {
+    failed_bundles_.insert(extension_id);
+    ScopedDictPrefUpdate update(profile_->GetPrefs(),
+                                prefs::kExtensionInstallGrace);
+    update->Remove(extension_id);
+  }
+  if (extension_id == primary_id_) {
+    FinishReadiness(false);
+  }
+}
+
+void BrowserOSExtensionLoader::OnShutdown(
+    extensions::ExtensionRegistry* registry) {
+  Shutdown();
+}
+
+void BrowserOSExtensionLoader::OnShutdown() {
+  Shutdown();
+}
+
+void BrowserOSExtensionLoader::OnProfileWillBeDestroyed(Profile* profile) {
+  Shutdown();
+}
+
+void BrowserOSExtensionLoader::Shutdown() {
+  if (!profile_) {
+    return;
+  }
+  // Cancel replies and observations while profile services are still alive.
+  // ExternalLoader itself may outlive them through the provider's references.
+  weak_ptr_factory_.InvalidateWeakPtrs();
+  FinishReadiness(false);
+  maintenance_timer_.Stop();
+  install_grace_timer_.Stop();
+  maintainer_.reset();
+  installer_.reset();
+  registry_observation_.Reset();
+  install_observation_.Reset();
+  profile_observation_.Reset();
+  profile_->RemoveUserData(kLoaderReferenceKey);
+  profile_ = nullptr;
+}
+
+}  // namespace browseros
