"""视觉一批 1117 的纯文本口径验收（不编译、不起界面）：关于页四行合规信息和
「更新由风桥负责」一行接上了、译文编号对得上、侧栏光效的光粒还在。
跑法：python packages/browseros/tools/windrise/visual_1117_accept_test.py"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2] / "chromium_patches"


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


class AboutPageRows(unittest.TestCase):
    HTML = "chrome/browser/resources/settings/about_page/about_page.html.ts"
    TS = "chrome/browser/resources/settings/about_page/about_page.ts"
    PROVIDER = "chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc"

    def test_rows_in_template(self):
        html = read(self.HTML)
        for needle in ('id="windrisePrivacy"', 'id="windriseTerms"',
                       "$i18n{aboutPrivacyLabel}", "$i18n{aboutTermsLabel}",
                       "$i18n{aboutOperatorLine}", "$i18n{aboutIcpLine}",
                       "this.showIcp_", 'id="windriseUpdates"',
                       "$i18n{aboutWindriseUpdates}"):
            self.assertIn(needle, html, needle)

    def test_links_read_urls_from_data_source_only(self):
        ts = read(self.TS)
        self.assertIn("loadTimeData.getString('aboutPrivacyUrl')", ts)
        self.assertIn("loadTimeData.getString('aboutTermsUrl')", ts)
        self.assertIn("loadTimeData.getBoolean('aboutShowIcp')", ts)
        self.assertNotIn("windrisebridge", ts)

    def test_provider_keys(self):
        cc = read(self.PROVIDER)
        for key in ("aboutPrivacyLabel", "aboutTermsLabel", "aboutOperatorLine",
                    "aboutIcpLine", "aboutWindriseUpdates"):
            self.assertIn('"%s"' % key, cc, key)
        self.assertRegex(cc, r'AddBoolean\("aboutShowIcp", windrise_is_chinese\)')


class ChineseStrings(unittest.TestCase):
    # grit 消息编号（tools/grit grit.extern.tclib.GenerateMessageId 算的；用户协议带 meaning，
    # 避开已有「服务条款」那条同文消息）。
    EXPECTED = {
        "8804164990146287819": "隐私政策",
        "5227175505375620501": "用户协议",
        "1082780094008919076": '运营主体 <ph name="OPERATOR" />',
        "3711678947804234161": '备案号 <ph name="NUMBER" />',
        "1924827426511278125": "浏览器更新由风桥检查和安装",
    }

    def test_translations(self):
        xtb = read("chrome/app/resources/generated_resources_zh-CN.xtb")
        for mid, text in self.EXPECTED.items():
            self.assertIn('+<translation id="%s">%s</translation>' % (mid, text), xtb, mid)

    def test_grdp_messages(self):
        grdp = read("chrome/app/settings_strings.grdp")
        for name in ("PRIVACY", "TERMS", "OPERATOR", "ICP", "UPDATES"):
            self.assertIn("IDS_SETTINGS_ABOUT_PAGE_WINDRISE_" + name, grdp)
        self.assertIn('meaning="Windrise About page legal row"', grdp)


class GlowMotes(unittest.TestCase):
    def test_motes_drawn_inside_band(self):
        cc = read("chrome/browser/ui/views/frame/windrise_ai_section_view.cc")
        self.assertIn("PaintMotes(canvas, group, radius, kk, t, light_sidebar);", cc)
        self.assertIn("constexpr float kMoteSpacing = 16.0f;", cc)
        # 一半顺时针一半逆时针，不是一段绕圈
        self.assertIn("const float dir = (i % 2) ? -1.0f : 1.0f;", cc)
        # 浅色侧栏不透明纯白、靠变小淡出
        self.assertIn("SkColor4f::FromColor(SK_ColorWHITE)", cc)


if __name__ == "__main__":
    unittest.main()
