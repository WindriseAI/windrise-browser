diff --git a/chrome/browser/browseros/core/windrise_ai_state.h b/chrome/browser/browseros/core/windrise_ai_state.h
new file mode 100644
index 0000000000..1dfbf1a88b
--- /dev/null
+++ b/chrome/browser/browseros/core/windrise_ai_state.h
@@ -0,0 +1,144 @@
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
+// - 侧边栏上下分段：哪些标签是 AI 的（ai_tab_ids）与当前 AI 名；
+// - 左下「已连接 N 个 AI」卡片与面板：connections；
+// - 边缘光效：哪些标签正在被 AI 操作（active_tab_ids），以及 AI 每个动作
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
+// 侧边栏下段里的一个 AI。每个 AI 一组（组名 = AI 名），由
+// browserOS.setAiTabs 按 aiName 建立 / 整组替换 / 传空数组清掉；不同 AI 互不影响。
+struct AiSection {
+  std::u16string name;
+  // 扩展声明属于这个 AI 的标签（ExtensionTabUtil::GetTabId 的口径）。
+  std::vector<int> tab_ids;
+  // 这个 AI 在标签条模型里对应的标签组（固定标签不进组，所以可能比 tab_ids 少）。
+  std::optional<tab_groups::TabGroupId> group;
+};
+
+struct AiState {
+  // 最近一次推了非空标签列表的 AI 名；空 = 没有 AI 在用标签。
+  std::u16string active_ai_name;
+  // 面板里的行。
+  std::vector<AiConnection> connections;
+  // 所有 AI 的标签（各段 tab_ids 的并集）。
+  std::vector<int> ai_tab_ids;
+  // 正在被 AI 操作的标签。
+  std::vector<int> active_tab_ids;
+  // 下段里的各个 AI，按第一次出现的先后排（侧栏里先来的在最下面）。
+  std::vector<AiSection> sections;
+};
+
+// 侧边栏下段各个 AI 对应的标签组。分组本身由扩展推的标签列表决定（见
+// browserOS.setAiTabs），浏览器只记下哪几个组是 AI 的：竖向标签条在上段里不画
+// 这些组（由下段 WindriseAiSectionView 画），横向标签条里组头按段标题配色。
+// 组 id 全进程唯一，所以放在一个进程级集合里，各窗口各 AI 的组都在里面。
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
+  // setAiTabs：按 AI 名建立 / 替换这个 AI 的一段；tab_ids 为空就删掉这一段。
+  // group 是接口层在标签条模型里给它建好的组（没有可建的就传 nullopt）。
+  void SetAiSection(std::u16string ai_name,
+                    std::vector<int> tab_ids,
+                    std::optional<tab_groups::TabGroupId> group);
+  const AiSection* FindSection(const std::u16string& ai_name) const;
+  void SetConnections(std::vector<AiConnection> connections);
+  // 带名字只开关这个 AI；不带名字时保留旧的按标签开关行为。
+  void SetActivity(int tab_id,
+                   bool active,
+                   std::optional<std::u16string> ai_name = std::nullopt);
+  // 扩展每个 AI 动作调一次。只通知脉冲观察者，不改状态；某个标签没在被操作时
+  // 观察者自己忽略。
+  void PulseActivity(int tab_id, std::optional<std::pair<float, float>> point);
+
+  const AiState& state() const { return state_; }
+  bool IsAiTab(int tab_id) const;
+  bool IsActive(int tab_id) const;
+  bool IsOperating(const std::u16string& ai_name) const;
+
+  base::CallbackListSubscription AddObserver(base::RepeatingClosure callback);
+  // 脉冲观察者：回调里带载荷，和上面「状态变了」的无参观察者分开。
+  base::CallbackListSubscription AddPulseObserver(
+      base::RepeatingCallback<void(const AiPulse&)> callback);
+
+ private:
+  AiWindowState();
+
+  void NotifyIfChanged(bool changed);
+  // sections 变了以后重算 ai_tab_ids（并集）。
+  void RebuildAiTabIds();
+
+  struct Activity {
+    int tab_id;
+    // 未带 aiName 的旧调用仍归最近一次推标签的 AI。
+    std::optional<std::u16string> ai_name;
+  };
+  std::vector<Activity> activities_;
+
+  AiState state_;
+  base::RepeatingClosureList observers_;
+  base::RepeatingCallbackList<void(const AiPulse&)> pulse_observers_;
+};
+
+}  // namespace browseros::windrise
+
+#endif  // CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_AI_STATE_H_
\ No newline at end of file
