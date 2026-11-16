diff --git a/ui/base/webui/web_ui_util.cc b/ui/base/webui/web_ui_util.cc
index 46ca652..966b725 100644
--- a/ui/base/webui/web_ui_util.cc
+++ b/ui/base/webui/web_ui_util.cc
@@ -239,6 +239,13 @@ void AppendWebUiCssTextDefaults(std::string* html) {
 std::string GetFontFamily() {
   std::string font_family = l10n_util::GetStringUTF8(IDS_WEB_FONT_FAMILY);
 
+#if BUILDFLAG(IS_WIN)
+  // 风桥 内部页统一 HarmonyOS 字体链；没装就依次回退到系统的西文 / 中文
+  // 字体（IDS_WEB_FONT_FAMILY 在 Windows 上已经是 "Segoe UI, Microsoft YaHei"
+  // 一类）。字体文件本身由 在启动时加载，这里只管 CSS 里的 family 名。
+  font_family = "\"HarmonyOS Sans SC\", " + font_family;
+#endif  // BUILDFLAG(IS_WIN)
+
 #if BUILDFLAG(IS_LINUX)
   std::string font_name = ui::ResourceBundle::GetSharedInstance()
                               .GetFont(ui::ResourceBundle::BaseFont)
