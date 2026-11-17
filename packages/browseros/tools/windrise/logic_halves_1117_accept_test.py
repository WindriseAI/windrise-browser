"""逻辑半边 1117 的自测（不依赖界面、不编译）：合规信息数据源 + P1 悬停展开合入。

    python packages/browseros/tools/windrise/logic_halves_1117_accept_test.py

来由（11-17「逻辑半边补齐」）：
1. 关于页合规信息（基础功能盘点-浏览器-1116 第 1.1 / 13 条、R421）：隐私政策、
   用户协议按界面语言选站，运营主体和备案号照官网页脚原文。界面行由设计帮手补，
   这里只保证数据源和对界面暴露的键在、值对。
2. 统一编译清单核对时发现 browser-integ-1116-p123 的 04c96b122（P1 悬停展开横排）
   没在底座头里，已按补丁口径合入。这里守住「合丢了」这一类回退。

两条都是纯文本口径的验收（和 hooks1_accept_test.py 同法）：值错、键缺、注册丢，
用户会点到错误的政策页、或窄栏按钮布局回到已作废的旧做法。
"""

import re
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
PKG = HERE.parents[1]
PATCHES = PKG / "chromium_patches"


def read(rel):
    return (PATCHES / rel).read_text(encoding="utf-8")


class LegalLinks(unittest.TestCase):
    """补项一：合规信息数据源（url_constants.h + 设置 WebUI 暴露）。"""

    URL_CONSTANTS = "chrome/common/url_constants.h"
    PROVIDER = (
        "chrome/browser/ui/webui/settings/settings_localized_strings_provider.cc"
    )

    # 和官网页脚一致的四个地址（site-cn/build.mjs SITE_ORIGIN 与 altEn）。
    EXPECTED_URLS = {
        "kWindrisePrivacyChineseURL": "https://www.windrisebridge.cn/privacy",
        "kWindriseTermsChineseURL": "https://www.windrisebridge.cn/terms",
        "kWindrisePrivacyURL": "https://www.windrisebridge.com/privacy",
        "kWindriseTermsURL": "https://www.windrisebridge.com/terms",
    }
    OPERATOR = "邯郸市丛台区风起网络科技工作室（个体工商户）"
    ICP = "冀ICP备2026049007号-1"

    def test_url_constants_define_the_four_site_pages(self):
        text = read(self.URL_CONSTANTS)
        for name, url in self.EXPECTED_URLS.items():
            self.assertRegex(
                text,
                r'\+inline constexpr char %s\[\] =\n\+    "%s";'
                % (re.escape(name), re.escape(url)),
                name,
            )

    def test_about_page_exposes_legal_keys_and_locale_rule(self):
        text = read(self.PROVIDER)
        for key in ("aboutPrivacyUrl", "aboutTermsUrl", "aboutOperator", "aboutIcp"):
            self.assertIn('"%s"' % key, text, key)
        # 语言判法必须和菜单帮助同一套（chrome_pages.cc）：zh / zh-* 走中文站。
        self.assertRegex(text, r"EqualsCaseInsensitiveASCII\(\s*windrise_locale,\s*\"zh\"\)")
        self.assertRegex(text, r"StartsWith\(\s*windrise_locale,\s*\"zh-\"")
        self.assertIn("GetApplicationLocale", text)

    def test_operator_and_icp_are_the_registered_values(self):
        text = read(self.PROVIDER)
        self.assertIn('u"%s"' % self.OPERATOR, text)
        self.assertIn('u"%s"' % self.ICP, text)

    def test_addresses_come_only_from_url_constants(self):
        # 暴露层不许另写政策地址：值只在 url_constants.h 出现一次，防止两处漂移。
        text = read(self.PROVIDER)
        for url in self.EXPECTED_URLS.values():
            self.assertNotIn(url, text, url)
        added = [line for line in text.splitlines() if line.startswith("+")]
        self.assertFalse(
            [line for line in added if "https://" in line and "windrisebridge" in line],
            "provider patch hardcodes a windrisebridge address",
        )


class P1HoverMerge(unittest.TestCase):
    """补项二：04c96b122（悬停展开横排）合入不许丢。"""

    COMBO_CC = "chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.cc"
    COMBO_H = "chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.h"
    TOP_CC = (
        "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_top_container.cc"
    )

    def test_combo_button_morphs_between_orientations(self):
        cc = read(self.COMBO_CC)
        header = read(self.COMBO_H)
        self.assertIn("void TabStripComboButton::SetOrientationTransitionProgress", cc)
        self.assertIn("orientation_transition_progress_", cc)
        self.assertIn("void SetOrientationTransitionProgress(double progress);", header)
        # 两按钮仍不显示标签（55208b8cb 的做法，回退会露「…」）。
        self.assertIn("+  button->SetShouldShowLabel(false);", cc)

    def test_top_container_interpolates_from_collapsed_position(self):
        text = read(self.TOP_CC)
        self.assertIn("GetExpandOnHoverProgress", text)
        self.assertIn("SetOrientationTransitionProgress", text)
        self.assertIn("CollapsedContentWidth", text)

    def test_registered_in_feature_registry(self):
        text = read(".features.yaml")
        block = re.search(
            r"  windrise-browser-p1-hover-1116:.*?(?=\n  \S|\Z)", text, re.S
        )
        self.assertIsNotNone(block, "windrise-browser-p1-hover-1116 missing")
        for rel in (self.COMBO_CC, self.COMBO_H, self.TOP_CC):
            self.assertIn('"%s"' % rel, block.group(0), rel)


if __name__ == "__main__":
    unittest.main()
