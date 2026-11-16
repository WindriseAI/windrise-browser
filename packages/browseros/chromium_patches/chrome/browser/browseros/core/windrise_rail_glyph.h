diff --git a/chrome/browser/browseros/core/windrise_rail_glyph.h b/chrome/browser/browseros/core/windrise_rail_glyph.h
new file mode 100644
index 0000000000..2f3d2376b1
--- /dev/null
+++ b/chrome/browser/browseros/core/windrise_rail_glyph.h
@@ -0,0 +1,35 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#ifndef CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_RAIL_GLYPH_H_
+#define CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_RAIL_GLYPH_H_
+
+#include <string>
+#include <string_view>
+
+// 风桥 theme-v2 §4：侧栏收起（70 宽）时每个标签在图标右边露出的那一个字。
+// 竖向标签（tabs/common/tab_view.cc）和侧栏下段的 AI 组合页
+// （frame/windrise_ai_section_view.cc）共用这一条规则。
+//
+// 规则照规格字面：
+//   1. 标题里有汉字，取第一个汉字（ICU 文字属 Han，含增补平面）；
+//   2. 否则取第一个英文（拉丁）字母，转大写；
+//   3. 两者都没有，取标题的第一个字符（跳过空白和不可见的格式字符），数字、
+//      假名、西里尔、符号、emoji 都照原样；
+//   4. 标题为空（或只有空白）时，对域名（去掉开头的 www.）按 1–3 取。
+// 例：「58同城」→「同」，「12306」→「1」，「(3) Inbox」→「I」，「github」→「G」，
+// 「お知らせ」→「知」（知 是汉字），「ありがとう」→「あ」，「Привет」→「П」，
+// 「【公告】系统维护」→「公」。
+// 都按码点走，不按 UTF-16 单元：增补平面的字不会被切成半个代理对。
+namespace browseros::windrise {
+
+// 只看标题；取不到（空或全是空白）返回空串。
+std::u16string RailGlyphForTitle(std::u16string_view title);
+
+// 标题取不到时退到域名。域名也取不到（about:blank 等）返回空串，调用方只显示
+// 图标。
+std::u16string RailGlyphFor(std::u16string_view title, std::string_view host);
+
+}  // namespace browseros::windrise
+
+#endif  // CHROME_BROWSER_BROWSEROS_CORE_WINDRISE_RAIL_GLYPH_H_
