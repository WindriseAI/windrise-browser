diff --git a/chrome/browser/browseros/core/windrise_ai_state.cc b/chrome/browser/browseros/core/windrise_ai_state.cc
new file mode 100644
index 0000000000..71f2445158
--- /dev/null
+++ b/chrome/browser/browseros/core/windrise_ai_state.cc
@@ -0,0 +1,194 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/browseros/core/windrise_ai_state.h"
+
+#include <algorithm>
+#include <map>
+#include <vector>
+#include <memory>
+#include <optional>
+#include <utility>
+
+#include "base/no_destructor.h"
+#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
+
+namespace browseros::windrise {
+namespace {
+
+struct Entry {
+  std::unique_ptr<AiWindowState> state;
+  int refs = 0;
+};
+
+// 窗口 → 记录。只在 UI 线程用；记录随最后一个视图 Detach 一起删，不会指向
+// 已经销毁的窗口。
+std::map<BrowserWindowInterface*, Entry>& GetRegistry() {
+  static base::NoDestructor<std::map<BrowserWindowInterface*, Entry>> registry;
+  return *registry;
+}
+
+std::vector<tab_groups::TabGroupId>& GetAiGroups() {
+  static base::NoDestructor<std::vector<tab_groups::TabGroupId>> groups;
+  return *groups;
+}
+
+bool HasTabId(const std::vector<int>& ids, int tab_id) {
+  return std::ranges::contains(ids, tab_id);
+}
+
+void SetTabId(std::vector<int>* ids, int tab_id, bool present) {
+  const auto it = std::ranges::find(*ids, tab_id);
+  if (present && it == ids->end()) {
+    ids->push_back(tab_id);
+  } else if (!present && it != ids->end()) {
+    ids->erase(it);
+  }
+}
+
+}  // namespace
+
+// static
+void AiGroups::Set(std::vector<tab_groups::TabGroupId> groups) {
+  GetAiGroups() = std::move(groups);
+}
+
+// static
+void AiGroups::Add(const tab_groups::TabGroupId& group) {
+  if (!std::ranges::contains(GetAiGroups(), group)) {
+    GetAiGroups().push_back(group);
+  }
+}
+
+// static
+void AiGroups::Remove(const tab_groups::TabGroupId& group) {
+  std::erase(GetAiGroups(), group);
+}
+
+// static
+bool AiGroups::Contains(const tab_groups::TabGroupId& group) {
+  return std::ranges::contains(GetAiGroups(), group);
+}
+
+AiWindowState::AiWindowState(BrowserWindowInterface* browser)
+    : browser_(browser) {}
+
+AiWindowState::~AiWindowState() {
+  if (state_.ai_group) {
+    AiGroups::Remove(*state_.ai_group);
+  }
+}
+
+// static
+AiWindowState* AiWindowState::From(BrowserWindowInterface* browser) {
+  if (!browser) {
+    return nullptr;
+  }
+  auto& registry = GetRegistry();
+  const auto it = registry.find(browser);
+  return it == registry.end() ? nullptr : it->second.state.get();
+}
+
+// static
+AiWindowState* AiWindowState::Attach(BrowserWindowInterface* browser) {
+  if (!browser) {
+    return nullptr;
+  }
+  Entry& entry = GetRegistry()[browser];
+  ++entry.refs;
+  if (!entry.state) {
+    // 构造函数是私有的，make_unique 访问不了，这里在成员函数里直接 new。
+    entry.state.reset(new AiWindowState(browser));
+  }
+  return entry.state.get();
+}
+
+// static
+void AiWindowState::Detach(BrowserWindowInterface* browser) {
+  if (!browser) {
+    return;
+  }
+  auto& registry = GetRegistry();
+  const auto it = registry.find(browser);
+  if (it == registry.end()) {
+    return;
+  }
+  if (--it->second.refs <= 0) {
+    registry.erase(it);
+  }
+}
+
+void AiWindowState::SetAiTabs(std::u16string ai_name,
+                              std::vector<int> tab_ids) {
+  const bool changed = state_.active_ai_name != ai_name ||
+                       state_.ai_tab_ids != tab_ids;
+  if (!changed) {
+    return;
+  }
+  state_.active_ai_name = std::move(ai_name);
+  state_.ai_tab_ids = std::move(tab_ids);
+  NotifyIfChanged(true);
+}
+
+void AiWindowState::SetAiGroup(std::optional<tab_groups::TabGroupId> group) {
+  if (state_.ai_group == group) {
+    return;
+  }
+  // 窗口各自一个 AI 组：换组 / 清空时把旧组从进程级集合里摘掉。
+  if (state_.ai_group) {
+    AiGroups::Remove(*state_.ai_group);
+  }
+  state_.ai_group = group;
+  if (group) {
+    AiGroups::Add(*group);
+  }
+}
+
+void AiWindowState::SetConnections(std::vector<AiConnection> connections) {
+  if (connections.size() == state_.connections.size()) {
+    bool same = true;
+    for (size_t i = 0; i < connections.size(); ++i) {
+      const AiConnection& a = connections[i];
+      const AiConnection& b = state_.connections[i];
+      if (a.name != b.name || a.icon_png != b.icon_png ||
+          a.last_active_minutes != b.last_active_minutes) {
+        same = false;
+        break;
+      }
+    }
+    if (same) {
+      return;
+    }
+  }
+  state_.connections = std::move(connections);
+  NotifyIfChanged(true);
+}
+
+void AiWindowState::SetActivity(int tab_id, bool active) {
+  if (HasTabId(state_.active_tab_ids, tab_id) == active) {
+    return;
+  }
+  SetTabId(&state_.active_tab_ids, tab_id, active);
+  NotifyIfChanged(true);
+}
+
+bool AiWindowState::IsAiTab(int tab_id) const {
+  return HasTabId(state_.ai_tab_ids, tab_id);
+}
+
+bool AiWindowState::IsActive(int tab_id) const {
+  return HasTabId(state_.active_tab_ids, tab_id);
+}
+
+base::CallbackListSubscription AiWindowState::AddObserver(
+    base::RepeatingClosure callback) {
+  return observers_.Add(std::move(callback));
+}
+
+void AiWindowState::NotifyIfChanged(bool changed) {
+  if (changed) {
+    observers_.Notify();
+  }
+}
+
+}  // namespace browseros::windrise
\ No newline at end of file
