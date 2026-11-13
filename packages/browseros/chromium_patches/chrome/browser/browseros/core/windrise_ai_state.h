diff --git a/chrome/browser/browseros/core/windrise_ai_state.h b/chrome/browser/browseros/core/windrise_ai_state.h
new file mode 100644
index 0000000000..d74f8aed28
--- /dev/null
+++ b/chrome/browser/browseros/core/windrise_ai_state.h
@@ -0,0 +1,118 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_AI_STATE_H_
+#define CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_AI_STATE_H_
+
+#include <optional>
+#include <string>
+#include <utility>
+#include <vector>
+
+#include "base/callback_list.h"
+#include "base/functional/callback.h"
+#include "base/memory/raw_ptr.h"
+#include "base/time/time.h"
+
+#include "components/tab_groups/tab_group_id.h"
+
+class BrowserWindowInterface;
+
+// 扩展推给浏览器的 AI 运行状态，按浏览器窗口保存。三个消费点：
+// - N6 侧边栏上下分段：哪些标签是 AI 的（ai_tab_ids）与当前 AI 名；
+// - N7 左下「已连接 N 个 AI」卡片与面板：connections；
+// - N11 边缘光效：哪些标签正在被 AI 操作（active_tab_ids），以及 AI 每个动作
+//   一次的脉冲（PulseActivity，不进 AiState：脉冲是一次性事件，不是状态）。
+//
+// 生命周期：窗口里的视图用 Attach()/Detach() 成对登记（引用计数），最后一个视图
+// 走掉时整条记录连同数据一起删。扩展接口只读 Get()，视图层只听 AddObserver()，
+// 所以不需要额外的 keyed service。状态只在扩展主动推送时变，浏览器不猜。
+namespace browseros::windrise {
+
+// 一家已连接的 AI。icon_png 是扩展推来的 PNG（16x16 以内），空表示没有图标；
+// 原生解码后用 gfx::ImageSkia 画，不联网、不查 favicon。
+struct AiConnection {
+  std::u16string name;
+  std::string icon_png;
+  int last_active_minutes = 0;
+};
+
+struct AiState {
+  // 「<AI 名> 正在操作」里的那个名字；空 = 没有 AI 在操作。
+  std::u16string active_ai_name;
+  // N7 面板里的行。
+  std::vector<AiConnection> connections;
+  // 扩展声明属于 AI 的标签（ExtensionTabUtil::GetTabId 的口径）。
+  std::vector<int> ai_tab_ids;
+  // 正在被 AI 操作的标签。
+  std::vector<int> active_tab_ids;
+  // 本窗口里「AI 那一段」对应的标签组（由 setAiTabs 建立）。
+  std::optional<tab_groups::TabGroupId> ai_group;
+};
+
+// 侧边栏「AI 那一段」对应的标签组。分组本身由扩展推的标签列表决定（见
+// browserOS.setAiTabs），浏览器只记下哪几个组是 AI 的，供标签组标题按段标题
+// 配色（kColorWindriseSectionHeaderText）渲染。按窗口会变，但组 id 全进程唯一，
+// 所以放在一个进程级集合里，跨窗口当前也都只有一组。
+class AiGroups {
+ public:
+  static void Set(std::vector<tab_groups::TabGroupId> groups);
+  static void Add(const tab_groups::TabGroupId& group);
+  static void Remove(const tab_groups::TabGroupId& group);
+  static bool Contains(const tab_groups::TabGroupId& group);
+};
+
+// AI 的一次动作（点击、输入、翻页、滚动）。位置是网页里的坐标（逻辑像素，
+// 相对网页区左上角），扩展没给就 has_point = false。
+struct AiPulse {
+  int tab_id = 0;
+  bool has_point = false;
+  float x = 0.0f;
+  float y = 0.0f;
+  base::TimeTicks time;
+};
+
+class AiWindowState {
+ public:
+  // 取已有记录；窗口里还没有视图登记时返回 nullptr。
+  static AiWindowState* From(BrowserWindowInterface* browser);
+  // 视图构造 / 析构时成对调用：引用计数归零时删记录。
+  static AiWindowState* Attach(BrowserWindowInterface* browser);
+  static void Detach(BrowserWindowInterface* browser);
+
+  AiWindowState(const AiWindowState&) = delete;
+  AiWindowState& operator=(const AiWindowState&) = delete;
+  ~AiWindowState();
+
+  // 扩展接口的写入点。值没变时不通知。
+  void SetAiTabs(std::u16string ai_name, std::vector<int> tab_ids);
+  void SetAiGroup(std::optional<tab_groups::TabGroupId> group);
+  void SetConnections(std::vector<AiConnection> connections);
+  void SetActivity(int tab_id, bool active);
+  // 扩展每个 AI 动作调一次。只通知脉冲观察者，不改状态；某个标签没在被操作时
+  // 观察者自己忽略。
+  void PulseActivity(int tab_id, std::optional<std::pair<float, float>> point);
+
+  const AiState& state() const { return state_; }
+  bool IsAiTab(int tab_id) const;
+  bool IsActive(int tab_id) const;
+
+  base::CallbackListSubscription AddObserver(base::RepeatingClosure callback);
+  // 脉冲观察者：回调里带载荷，和上面「状态变了」的无参观察者分开。
+  base::CallbackListSubscription AddPulseObserver(
+      base::RepeatingCallback<void(const AiPulse&)> callback);
+
+ private:
+  explicit AiWindowState(BrowserWindowInterface* browser);
+
+  void NotifyIfChanged(bool changed);
+
+  raw_ptr<BrowserWindowInterface> browser_;
+  AiState state_;
+  base::RepeatingClosureList observers_;
+  base::RepeatingCallbackList<void(const AiPulse&)> pulse_observers_;
+};
+
+}  // namespace browseros::windrise
+
+#endif  // CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_AI_STATE_H_
\ No newline at end of file
