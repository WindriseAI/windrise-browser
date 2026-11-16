diff --git a/chrome/browser/browseros/core/windrise_rail_glyph.cc b/chrome/browser/browseros/core/windrise_rail_glyph.cc
new file mode 100644
index 0000000000..ac31d7b27a
--- /dev/null
+++ b/chrome/browser/browseros/core/windrise_rail_glyph.cc
@@ -0,0 +1,69 @@
+// Copyright 2026 Windrise. Use of this source code is governed by the
+// AGPL-3.0 license that can be found in the LICENSE file.
+
+#include "chrome/browser/browseros/core/windrise_rail_glyph.h"
+
+#include "base/i18n/char_iterator.h"
+#include "base/strings/utf_string_conversion_utils.h"
+#include "base/strings/utf_string_conversions.h"
+#include "third_party/icu/source/common/unicode/uchar.h"
+#include "third_party/icu/source/common/unicode/uscript.h"
+
+namespace browseros::windrise {
+namespace {
+
+std::u16string FromCodePoint(base_icu::UChar32 c) {
+  std::u16string glyph;
+  base::WriteUnicodeCharacter(c, &glyph);
+  return glyph;
+}
+
+UScriptCode ScriptOf(base_icu::UChar32 c) {
+  UErrorCode status = U_ZERO_ERROR;
+  const UScriptCode script = uscript_getScript(c, &status);
+  return U_SUCCESS(status) ? script : USCRIPT_INVALID_CODE;
+}
+
+// 空白、控制字符、零宽连接符 / BOM 这类格式字符都不算「一个字」。
+bool IsVisible(base_icu::UChar32 c) {
+  return !u_isUWhiteSpace(c) && !u_iscntrl(c) &&
+         u_charType(c) != U_FORMAT_CHAR;
+}
+
+}  // namespace
+
+std::u16string RailGlyphForTitle(std::u16string_view title) {
+  // 1. 第一个汉字。
+  for (base::i18n::UTF16CharIterator it(title); !it.end(); it.Advance()) {
+    if (ScriptOf(it.get()) == USCRIPT_HAN) {
+      return FromCodePoint(it.get());
+    }
+  }
+  // 2. 第一个英文字母，转大写。
+  for (base::i18n::UTF16CharIterator it(title); !it.end(); it.Advance()) {
+    const base_icu::UChar32 c = it.get();
+    if (u_isalpha(c) && ScriptOf(c) == USCRIPT_LATIN) {
+      return FromCodePoint(u_toupper(c));
+    }
+  }
+  // 3. 标题的第一个字符。
+  for (base::i18n::UTF16CharIterator it(title); !it.end(); it.Advance()) {
+    if (IsVisible(it.get())) {
+      return FromCodePoint(it.get());
+    }
+  }
+  return std::u16string();
+}
+
+std::u16string RailGlyphFor(std::u16string_view title, std::string_view host) {
+  std::u16string glyph = RailGlyphForTitle(title);
+  if (!glyph.empty()) {
+    return glyph;
+  }
+  if (host.starts_with("www.")) {
+    host.remove_prefix(4);
+  }
+  return RailGlyphForTitle(base::UTF8ToUTF16(host));
+}
+
+}  // namespace browseros::windrise
