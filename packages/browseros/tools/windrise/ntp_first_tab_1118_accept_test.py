"""新标签页首屏 1118 的自测（不依赖界面、不编译）：启动后第一个标签页必须落在风桥新标签页。

    python packages/browseros/tools/windrise/ntp_first_tab_1118_accept_test.py

来由（11-18「浏览器启动后第一个标签页打开的是谷歌的白色新标签页」）：
启动和首跑结束后第一个标签页都是 chrome://newtab/，它去哪只由扩展
chrome_url_overrides.newtab 的接管决定：ExtensionUrlOverridesRegistrar 在扩展加载时
注册覆盖，chrome://newtab 才会被重写到风桥助手的 newtab.html（助手 manifest.json 的
chrome_url_overrides.newtab，随主程序 --load-extension 加载），否则落到 Chromium 兜底
新标签页（默认引擎是 Google，白色那一张）。1118 查明：注册处的产品门闸只认
kBrowserOSExtensions 目录里的产品扩展，助手不在目录里，覆盖整份被丢掉，接管从没生效。
这里守住两头：覆盖注册放行已验证的助手；启动路径不许再冒别的首屏地址。
1118 续做（审核第四批第 10 笔）：助手只许接管 newtab 一项，它的别的 chrome://
覆盖和第三方扩展一样被门闸丢掉。
（风桥侧 manifest.json 的 chrome_url_overrides.newtab 是接管的另一半，归风桥仓库管。）
"""

import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
PKG = HERE.parents[1]
PATCHES = PKG / "chromium_patches"


def read(rel):
    return (PATCHES / rel).read_text(encoding="utf8")


class OverrideGate(unittest.TestCase):
    """补项一：覆盖注册的产品门闸只放行产品扩展和已验证的风桥助手。"""

    REGISTRAR = "chrome/browser/extensions/extension_url_overrides_registrar.cc"

    def test_verified_assistant_override_passes_the_gate(self):
        text = read(self.REGISTRAR)
        self.assertIn(
            "+    if (!IsVerifiedWindriseExtension(extension)) {",
            text,
            "assistant newtab override is dropped by the product gate again",
        )
        self.assertIn(
            '+#include "chrome/browser/extensions/windrise_extension_protection.h"',
            text,
        )

    def test_assistant_may_only_override_newtab(self):
        # 助手只许接管 newtab 一项：过滤只留 newtab 键，注册用过滤后的表。
        text = read(self.REGISTRAR)
        self.assertIn(
            "+      if (entry.first == chrome::kChromeUINewTabHost) {",
            text,
            "assistant overrides are no longer filtered down to newtab",
        )
        self.assertIn(
            "+      Profile::FromBrowserContext(browser_context), *allowed);",
            text,
            "registration must receive the filtered map, not the raw one",
        )

    def test_product_gate_still_drops_every_other_extension(self):
        text = read(self.REGISTRAR)
        self.assertIn(
            "+      !browseros::IsActiveBrowserOSExtension(extension->id())) {",
            text,
            "non-product extensions must stay unable to override chrome pages",
        )
        # 身份只在 windrise_extension_protection.cc 一处，注册处不许抄 ID。
        self.assertNotIn("amlnokjigkbgngdgkofikfphjbknfodp", text)


class StartupFirstTab(unittest.TestCase):
    """补项二：启动首标签只落 chrome://newtab/，由接管重写到我们的新标签页。"""

    STARTUP = "chrome/browser/ui/startup/startup_browser_creator.cc"
    ONBOARDING = "chrome/browser/browseros/onboarding/browseros_onboarding.cc"

    @staticmethod
    def stray_addresses(patch_text):
        stray = []
        for line in patch_text.splitlines():
            if not line.startswith("+"):
                continue
            if "http://" in line or "https://" in line:
                stray.append(line)
            elif "chrome://" in line.replace("chrome://newtab/", ""):
                stray.append(line)
        return stray

    def test_first_run_handoff_only_opens_newtab(self):
        text = read(self.STARTUP)
        self.assertIn('+      tabs.emplace_back("chrome://newtab/");', text)
        # 启动 / 首跑的标签清单里不许出现别的地址：那会变成「启动页被设成
        # 别的地址」，新标签页接管再准也拦不住。
        self.assertFalse(
            self.stray_addresses(text),
            "startup patch adds a first tab address that is not the new tab page",
        )

    def test_startup_tab_selection_is_not_rewritten(self):
        # 上游 DetermineStartupTabs 不许被改写：默认「打开新标签页」仍是首屏来源，
        # 启动页设置（kRestoreOnStartup / kURLsToRestoreOnStartup）不许被本仓补丁写。
        text = read(self.STARTUP)
        self.assertNotIn("DetermineStartupTabs", text)
        self.assertNotIn("kRestoreOnStartup", text)
        self.assertNotIn("kURLsToRestoreOnStartup", text)

    def test_onboarding_redirects_only_to_newtab(self):
        text = read(self.ONBOARDING)
        self.assertIn("GURL(chrome::kChromeUINewTabURL)", text)
        self.assertFalse(
            self.stray_addresses(text),
            "onboarding redirect introduces an address outside the new tab page",
        )


if __name__ == "__main__":
    unittest.main()
