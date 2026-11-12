diff --git a/chrome/browser/extensions/api/browser_os/browser_os_api.cc b/chrome/browser/extensions/api/browser_os/browser_os_api.cc
new file mode 100644
index 0000000..165bc45
--- /dev/null
+++ b/chrome/browser/extensions/api/browser_os/browser_os_api.cc
@@ -0,0 +1,590 @@
+// Copyright 2024 The Chromium Authors
+// Use of this source code is governed by a BSD-style license that can be
+// found in the LICENSE file.
+
+#include "chrome/browser/extensions/api/browser_os/browser_os_api.h"
+
+#include <algorithm>
+#include <memory>
+#include <optional>
+#include <string>
+#include <utility>
+
+#include "base/base64.h"
+#include "base/files/file_util.h"
+#include "base/logging.h"
+#include "base/strings/stringprintf.h"
+#include "base/strings/utf_string_conversions.h"
+#include "base/time/time.h"
+#include "base/values.h"
+#include "base/version_info/version_info.h"
+#include "chrome/browser/browser_process.h"
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+#include "chrome/browser/browseros/metrics/browseros_metrics.h"
+#include "chrome/browser/infobars/confirm_infobar_creator.h"
+#include "chrome/browser/platform_util.h"
+#include "chrome/browser/profiles/profile.h"
+#include "chrome/browser/ui/actions/chrome_action_id.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
+#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
+#include "chrome/browser/ui/page_action/page_action_controller.h"
+#include "chrome/browser/ui/select_file_policy/chrome_select_file_policy.h"
+#include "chrome/browser/ui/tabs/public/tab_features.h"
+#include "chrome/browser/ui/tabs/tab_group_model.h"
+#include "chrome/browser/ui/tabs/tab_strip_model.h"
+#include "chrome/browser/ui/toasts/api/toast_id.h"
+#include "chrome/browser/ui/toasts/toast_controller.h"
+#include "chrome/common/extensions/api/browser_os.h"
+#include "chrome/grit/generated_resources.h"
+#include "components/infobars/content/content_infobar_manager.h"
+#include "components/infobars/core/infobar.h"
+#include "components/infobars/core/infobar_delegate.h"
+#include "components/infobars/core/simple_alert_infobar_delegate.h"
+#include "components/prefs/pref_service.h"
+#include "components/tab_groups/tab_group_visual_data.h"
+#include "components/tabs/public/tab_group.h"
+#include "components/tabs/public/tab_interface.h"
+#include "content/public/browser/web_contents.h"
+#include "chrome/browser/extensions/extension_tab_util.h"
+#include "ui/base/l10n/l10n_util.h"
+#include "ui/shell_dialogs/selected_file_info.h"
+
+namespace extensions {
+namespace api {
+
+namespace {
+
+PrefService* FindPrefService(const std::string& pref_name, Profile* profile) {
+  PrefService* local_state = g_browser_process->local_state();
+  if (local_state && local_state->FindPreference(pref_name)) {
+    return local_state;
+  }
+
+  PrefService* profile_prefs = profile->GetPrefs();
+  if (profile_prefs && profile_prefs->FindPreference(pref_name)) {
+    return profile_prefs;
+  }
+
+  return nullptr;
+}
+
+// 按标签 id 找到窗口（与 setAddressBarSlot 同一套查法）。
+BrowserWindowInterface* FindBrowserForTabId(Profile* profile, int tab_id) {
+  ProfileBrowserCollection* browser_collection =
+      ProfileBrowserCollection::GetForProfile(profile);
+  if (!browser_collection) {
+    return nullptr;
+  }
+  BrowserWindowInterface* found = nullptr;
+  browser_collection->ForEach(
+      [&found, tab_id](BrowserWindowInterface* browser) {
+        TabStripModel* tab_strip_model = browser->GetTabStripModel();
+        if (!tab_strip_model) {
+          return true;
+        }
+        for (int i = 0; i < tab_strip_model->count(); ++i) {
+          content::WebContents* contents = tab_strip_model->GetWebContentsAt(i);
+          if (contents &&
+              extensions::ExtensionTabUtil::GetTabId(contents) == tab_id) {
+            found = browser;
+            return false;
+          }
+        }
+        return true;
+      },
+      BrowserCollection::Order::kActivation);
+  return found;
+}
+
+std::string GetPrefTypeName(const base::Value* value) {
+  switch (value->type()) {
+    case base::Value::Type::BOOLEAN:
+      return "boolean";
+    case base::Value::Type::INTEGER:
+    case base::Value::Type::DOUBLE:
+      return "number";
+    case base::Value::Type::STRING:
+      return "string";
+    case base::Value::Type::LIST:
+      return "list";
+    case base::Value::Type::DICT:
+      return "dictionary";
+    default:
+      return "unknown";
+  }
+}
+
+}  // namespace
+
+ExtensionFunction::ResponseAction BrowserOSGetPrefFunction::Run() {
+  std::optional<browser_os::GetPref::Params> params =
+      browser_os::GetPref::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  Profile* profile = Profile::FromBrowserContext(browser_context());
+  PrefService* prefs = FindPrefService(params->name, profile);
+
+  if (!prefs) {
+    return RespondNow(Error("Preference not found: " + params->name));
+  }
+
+  browser_os::PrefObject pref_obj;
+  pref_obj.key = params->name;
+
+  // The effective value, so values forced by policy or the command line
+  // (e.g. --disable-browseros-features) show through.
+  const base::Value* value = prefs->FindPreference(params->name)->GetValue();
+
+  pref_obj.type = GetPrefTypeName(value);
+  pref_obj.value = value->Clone();
+
+  return RespondNow(
+      ArgumentList(browser_os::GetPref::Results::Create(pref_obj)));
+}
+
+ExtensionFunction::ResponseAction BrowserOSSetPrefFunction::Run() {
+  std::optional<browser_os::SetPref::Params> params =
+      browser_os::SetPref::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  // Security: only allow modifying browseros.* prefs
+  if (!params->name.starts_with("browseros.")) {
+    return RespondNow(Error("Only browseros.* preferences can be modified"));
+  }
+
+  Profile* profile = Profile::FromBrowserContext(browser_context());
+  PrefService* prefs = FindPrefService(params->name, profile);
+
+  if (!prefs) {
+    return RespondNow(Error("Preference not found: " + params->name));
+  }
+
+  prefs->Set(params->name, params->value);
+
+  return RespondNow(ArgumentList(browser_os::SetPref::Results::Create(true)));
+}
+
+ExtensionFunction::ResponseAction BrowserOSLogMetricFunction::Run() {
+  std::optional<browser_os::LogMetric::Params> params =
+      browser_os::LogMetric::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  const std::string& event_name = params->event_name;
+
+  // Add "extension." prefix to distinguish from native events
+  std::string prefixed_event = "extension." + event_name;
+
+  if (params->properties.has_value()) {
+    // The properties parameter is a Properties struct with
+    // additional_properties member
+    base::DictValue properties =
+        params->properties->additional_properties.Clone();
+
+    properties.Set("extension_id", extension_id());
+
+    browseros_metrics::BrowserOSMetrics::Log(prefixed_event,
+                                             std::move(properties));
+  } else {
+    base::DictValue properties;
+    properties.Set("extension_id", extension_id());
+    browseros_metrics::BrowserOSMetrics::Log(prefixed_event,
+                                             std::move(properties));
+  }
+
+  return RespondNow(NoArguments());
+}
+
+ExtensionFunction::ResponseAction BrowserOSGetVersionNumberFunction::Run() {
+  std::string version = std::string(version_info::GetVersionNumber());
+
+  return RespondNow(
+      ArgumentList(browser_os::GetVersionNumber::Results::Create(version)));
+}
+
+ExtensionFunction::ResponseAction
+BrowserOSGetBrowserosVersionNumberFunction::Run() {
+  std::string version = std::string(version_info::GetBrowserOSVersionNumber());
+
+  return RespondNow(ArgumentList(
+      browser_os::GetBrowserosVersionNumber::Results::Create(version)));
+}
+
+namespace {
+
+constexpr char kCouldNotShowSelectFileDialogError[] =
+    "Could not show file dialog";
+
+ui::SelectFileDialog::Type GetDialogType(
+    const std::optional<browser_os::SelectionType>& type) {
+  if (type.has_value() && *type == browser_os::SelectionType::kFolder) {
+    return ui::SelectFileDialog::SELECT_FOLDER;
+  }
+  return ui::SelectFileDialog::SELECT_OPEN_FILE;
+}
+
+}  // namespace
+
+BrowserOSChoosePathFunction::BrowserOSChoosePathFunction() = default;
+
+BrowserOSChoosePathFunction::~BrowserOSChoosePathFunction() {
+  // Clean up pending file dialogs to prevent callbacks to a destroyed object.
+  if (select_file_dialog_.get()) {
+    select_file_dialog_->ListenerDestroyed();
+  }
+}
+
+ExtensionFunction::ResponseAction BrowserOSChoosePathFunction::Run() {
+  std::optional<browser_os::ChoosePath::Params> params =
+      browser_os::ChoosePath::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  content::WebContents* web_contents = GetSenderWebContents();
+  if (!web_contents) {
+    return RespondNow(Error(kCouldNotShowSelectFileDialogError));
+  }
+
+  ui::SelectFileDialog::Type dialog_type =
+      ui::SelectFileDialog::SELECT_OPEN_FILE;
+  std::u16string title;
+  base::FilePath starting_path;
+
+  if (params->options) {
+    dialog_type = GetDialogType(params->options->type);
+
+    if (params->options->title) {
+      title = base::UTF8ToUTF16(*params->options->title);
+    }
+
+    if (params->options->starting_directory) {
+      starting_path =
+          base::FilePath::FromUTF8Unsafe(*params->options->starting_directory);
+      // Validate path exists; if not, use empty path (OS default)
+      if (!base::DirectoryExists(starting_path)) {
+        starting_path = base::FilePath();
+      }
+    }
+  }
+
+  gfx::NativeWindow owning_window =
+      platform_util::GetTopLevel(web_contents->GetNativeView());
+
+  select_file_dialog_ = ui::SelectFileDialog::Create(
+      this, std::make_unique<ChromeSelectFilePolicy>(web_contents));
+
+  select_file_dialog_->SelectFile(
+      dialog_type, title, starting_path,
+      nullptr,                       // file_types
+      0,                             // file_type_index
+      base::FilePath::StringType(),  // default_extension
+      owning_window);
+
+  // prevent destruction while dialog is showing
+  AddRef();
+  return RespondLater();
+}
+
+void BrowserOSChoosePathFunction::FileSelected(const ui::SelectedFileInfo& file,
+                                               int index) {
+  browser_os::SelectedPath result;
+  result.path = file.path().AsUTF8Unsafe();
+  result.name = file.path().BaseName().AsUTF8Unsafe();
+
+  Respond(ArgumentList(browser_os::ChoosePath::Results::Create(result)));
+  Release();
+}
+
+void BrowserOSChoosePathFunction::FileSelectionCanceled() {
+  // Return null to indicate cancellation (not an error)
+  base::ListValue results;
+  results.Append(base::Value());
+  Respond(ArgumentList(std::move(results)));
+  Release();
+}
+
+ExtensionFunction::ResponseAction BrowserOSShowToastFunction::Run() {
+  std::optional<browser_os::ShowToast::Params> params =
+      browser_os::ShowToast::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  if (params->message.empty()) {
+    return RespondNow(Error("Toast message must not be empty"));
+  }
+
+  Profile* profile = Profile::FromBrowserContext(browser_context());
+  ProfileBrowserCollection* browser_collection =
+      ProfileBrowserCollection::GetForProfile(profile);
+  BrowserWindowInterface* browser =
+      browser_collection ? browser_collection->GetLastActiveBrowser() : nullptr;
+  if (!browser) {
+    return RespondNow(Error("No active browser window"));
+  }
+
+  ToastController* toast_controller = ToastController::From(browser);
+  if (!toast_controller) {
+    return RespondNow(Error("Toast controller unavailable"));
+  }
+
+  ToastParams toast_params(ToastId::kBrowserOSToast);
+  toast_params.body_string_override = base::UTF8ToUTF16(params->message);
+  if (params->duration_ms) {
+    constexpr int kMinDurationMs = 1000;
+    constexpr int kMaxDurationMs = 30000;
+    toast_params.timeout_override = base::Milliseconds(
+        std::clamp(*params->duration_ms, kMinDurationMs, kMaxDurationMs));
+  }
+
+  const bool shown = toast_controller->MaybeShowToast(std::move(toast_params));
+  return RespondNow(
+      ArgumentList(browser_os::ShowToast::Results::Create(shown)));
+}
+
+// Bridges chrome.browserOS.showInfoBar to Chrome's tab-scoped alert infobar.
+ExtensionFunction::ResponseAction BrowserOSShowInfoBarFunction::Run() {
+  std::optional<browser_os::ShowInfoBar::Params> params =
+      browser_os::ShowInfoBar::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  if (params->message.empty()) {
+    return RespondNow(Error("InfoBar message must not be empty"));
+  }
+
+  Profile* profile = Profile::FromBrowserContext(browser_context());
+  ProfileBrowserCollection* browser_collection =
+      ProfileBrowserCollection::GetForProfile(profile);
+  BrowserWindowInterface* browser =
+      browser_collection ? browser_collection->GetLastActiveBrowser() : nullptr;
+  if (!browser) {
+    return RespondNow(Error("No active browser window"));
+  }
+
+  content::WebContents* contents =
+      browser->GetTabStripModel()->GetActiveWebContents();
+  if (!contents) {
+    return RespondNow(Error("No active tab"));
+  }
+
+  infobars::ContentInfoBarManager* infobar_manager =
+      infobars::ContentInfoBarManager::FromWebContents(contents);
+  if (!infobar_manager) {
+    return RespondNow(Error("InfoBar manager unavailable"));
+  }
+
+  const bool shown =
+      infobar_manager
+          ->AddInfoBar(CreateConfirmInfoBar(
+              std::make_unique<SimpleAlertInfoBarDelegate>(
+                  infobars::InfoBarDelegate::
+                      BROWSEROS_EXTENSION_INFOBAR_DELEGATE,
+                  nullptr, base::UTF8ToUTF16(params->message),
+                  /*auto_expire=*/true, /*should_animate=*/true,
+                  /*closeable=*/true)))
+          != nullptr;
+
+  return RespondNow(
+      ArgumentList(browser_os::ShowInfoBar::Results::Create(shown)));
+}
+
+// Bridges chrome.browserOS.setAddressBarSlot to the tab-scoped page action
+// controller. State is kept per tab, so switching tabs preserves each slot.
+ExtensionFunction::ResponseAction BrowserOSSetAddressBarSlotFunction::Run() {
+  std::optional<browser_os::SetAddressBarSlot::Params> params =
+      browser_os::SetAddressBarSlot::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  Profile* profile = Profile::FromBrowserContext(browser_context());
+  ProfileBrowserCollection* browser_collection =
+      ProfileBrowserCollection::GetForProfile(profile);
+  if (!browser_collection) {
+    return RespondNow(Error("No browser window"));
+  }
+
+  ::tabs::TabInterface* target_tab = nullptr;
+  browser_collection->ForEach(
+      [&target_tab, &params](BrowserWindowInterface* browser) {
+        TabStripModel* tab_strip_model = browser->GetTabStripModel();
+        if (!tab_strip_model) {
+          return true;
+        }
+        for (int i = 0; i < tab_strip_model->count(); ++i) {
+          content::WebContents* contents =
+              tab_strip_model->GetWebContentsAt(i);
+          if (contents &&
+              extensions::ExtensionTabUtil::GetTabId(contents) ==
+                  params->options.tab_id) {
+            target_tab = tab_strip_model->GetTabAtIndex(i);
+            return false;
+          }
+        }
+        return true;
+      },
+      BrowserCollection::Order::kActivation);
+
+  if (!target_tab || !target_tab->GetTabFeatures()) {
+    return RespondNow(
+        Error(base::StringPrintf("No tab with id: %d", params->options.tab_id)));
+  }
+
+  page_actions::PageActionController* page_action_controller =
+      target_tab->GetTabFeatures()->page_action_controller();
+  if (!page_action_controller) {
+    return RespondNow(Error("Page action controller unavailable"));
+  }
+
+  if (params->options.visible) {
+    if (params->options.text) {
+      page_action_controller->OverrideText(
+          kActionWindriseSlot, base::UTF8ToUTF16(*params->options.text));
+    } else {
+      page_action_controller->ClearOverrideText(kActionWindriseSlot);
+    }
+    page_action_controller->Show(kActionWindriseSlot);
+    page_action_controller->ShowSuggestionChip(kActionWindriseSlot);
+  } else {
+    page_action_controller->HideSuggestionChip(kActionWindriseSlot);
+    page_action_controller->ClearOverrideText(kActionWindriseSlot);
+    page_action_controller->Hide(kActionWindriseSlot);
+  }
+
+  return RespondNow(NoArguments());
+}
+
+// 风桥 N6：哪些标签是当前 AI 的。浏览器把这些标签收成一个标签组，组标题就是
+// 侧边栏那一段的段标题（「<AI 名> 正在操作」），组排到最下面 = 下面那一段。
+// 组本身由扩展推的列表决定，浏览器不改用户的组名以外的任何东西。
+ExtensionFunction::ResponseAction BrowserOSSetAiTabsFunction::Run() {
+  std::optional<browser_os::SetAiTabs::Params> params =
+      browser_os::SetAiTabs::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  Profile* profile = Profile::FromBrowserContext(browser_context());
+  BrowserWindowInterface* browser =
+      FindBrowserForTabId(profile, params->options.tab_id);
+  if (!browser) {
+    return RespondNow(
+        Error(base::StringPrintf("No tab with id: %d", params->options.tab_id)));
+  }
+  browseros::windrise::AiWindowState* ai_state =
+      browseros::windrise::AiWindowState::From(browser);
+  if (!ai_state) {
+    return RespondNow(Error("No browser window state"));
+  }
+  TabStripModel* tab_strip_model = browser->GetTabStripModel();
+  if (!tab_strip_model) {
+    return RespondNow(Error("No tab strip"));
+  }
+
+  // 标签 id → 模型下标，升序（AddToNewGroup 要求）。
+  std::vector<int> indices;
+  for (int tab_id : params->options.tab_ids) {
+    for (int i = 0; i < tab_strip_model->count(); ++i) {
+      content::WebContents* contents = tab_strip_model->GetWebContentsAt(i);
+      if (contents && extensions::ExtensionTabUtil::GetTabId(contents) == tab_id) {
+        indices.push_back(i);
+        break;
+      }
+    }
+  }
+  std::ranges::sort(indices);
+
+  if (!indices.empty()) {
+    std::optional<tab_groups::TabGroupId> group_id =
+        tab_strip_model->GetTabAtIndex(indices.front())->GetGroup();
+    for (int index : indices) {
+      if (tab_strip_model->GetTabAtIndex(index)->GetGroup() != group_id) {
+        group_id = std::nullopt;
+        break;
+      }
+    }
+    if (!group_id) {
+      group_id = tab_strip_model->AddToNewGroup(indices);
+    }
+    if (group_id) {
+      tab_strip_model->ChangeTabGroupVisuals(
+          *group_id,
+          tab_groups::TabGroupVisualData(
+              l10n_util::GetStringFUTF16(
+                  IDS_WINDRISE_AI_OPERATING,
+                  base::UTF8ToUTF16(params->options.ai_name)),
+              tab_groups::TabGroupColorId::kGrey));
+      // 排到最后 = 侧边栏最下面那一段。
+      tab_strip_model->MoveGroupTo(*group_id, tab_strip_model->count());
+      browseros::windrise::AiGroups::Set({*group_id});
+    }
+  } else {
+    browseros::windrise::AiGroups::Set({});
+  }
+
+  ai_state->SetAiTabs(base::UTF8ToUTF16(params->options.ai_name),
+                      params->options.tab_ids);
+  return RespondNow(NoArguments());
+}
+
+// 风桥 N7：卡片 / 面板的数据。图标是 base64 PNG，原生自己解码，不联网。
+ExtensionFunction::ResponseAction BrowserOSSetConnectedAisFunction::Run() {
+  std::optional<browser_os::SetConnectedAis::Params> params =
+      browser_os::SetConnectedAis::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  Profile* profile = Profile::FromBrowserContext(browser_context());
+  BrowserWindowInterface* browser =
+      FindBrowserForTabId(profile, params->options.tab_id);
+  if (!browser) {
+    return RespondNow(
+        Error(base::StringPrintf("No tab with id: %d", params->options.tab_id)));
+  }
+  browseros::windrise::AiWindowState* ai_state =
+      browseros::windrise::AiWindowState::From(browser);
+  if (!ai_state) {
+    return RespondNow(Error("No browser window state"));
+  }
+
+  // 面板只画得下这么多行；图标也有上限，避免扩展推超大值。
+  constexpr size_t kMaxConnections = 8;
+  constexpr size_t kMaxIconBytes = 64 * 1024;
+  std::vector<browseros::windrise::AiConnection> connections;
+  for (const browser_os::AiConnection& input : params->options.connections) {
+    if (connections.size() >= kMaxConnections) {
+      break;
+    }
+    browseros::windrise::AiConnection connection;
+    connection.name = base::UTF8ToUTF16(input.name);
+    connection.last_active_minutes =
+        input.last_active_minutes.value_or(-1);
+    if (input.icon_png) {
+      std::string decoded;
+      if (base::Base64Decode(*input.icon_png, &decoded) &&
+          decoded.size() <= kMaxIconBytes) {
+        connection.icon_png = std::move(decoded);
+      }
+    }
+    connections.push_back(std::move(connection));
+  }
+  ai_state->SetConnections(std::move(connections));
+  return RespondNow(NoArguments());
+}
+
+// 风桥 N11：开关某个标签的银色海浪。
+ExtensionFunction::ResponseAction BrowserOSSetAiActivityFunction::Run() {
+  std::optional<browser_os::SetAiActivity::Params> params =
+      browser_os::SetAiActivity::Params::Create(args());
+  EXTENSION_FUNCTION_VALIDATE(params);
+
+  Profile* profile = Profile::FromBrowserContext(browser_context());
+  BrowserWindowInterface* browser =
+      FindBrowserForTabId(profile, params->options.tab_id);
+  if (!browser) {
+    return RespondNow(
+        Error(base::StringPrintf("No tab with id: %d", params->options.tab_id)));
+  }
+  browseros::windrise::AiWindowState* ai_state =
+      browseros::windrise::AiWindowState::From(browser);
+  if (!ai_state) {
+    return RespondNow(Error("No browser window state"));
+  }
+  ai_state->SetActivity(params->options.tab_id, params->options.active);
+  return RespondNow(NoArguments());
+}
+
+}  // namespace api
+}  // namespace extensions
