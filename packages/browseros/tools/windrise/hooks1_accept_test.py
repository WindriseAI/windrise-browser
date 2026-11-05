"""验收（Claude 写，实现方不许改）：浏览器编译前的小钩子：改名、c3 图标、关窗即退出。

    python packages/browseros/tools/windrise/hooks1_accept_test.py

来由（11-05，全流程图 6.1、6.12）：用户定浏览器叫 Windrise Browser（中文 Windrise 浏览器），
图标用 c3 套件，打开在任务栏、关了就关，不缩到右下角。

接口：
- BRANDING.release / BRANDING.debug 的 PRODUCT_FULLNAME、PRODUCT_SHORTNAME 是 Windrise Browser。
- bos_build/products/browseros/product.py 的 display_name 是 Windrise Browser。
- 数据和安装位置不改：chromium_install_modes.h、chrome_paths.cc 的补丁相对基线不变，
  风桥靠 %LOCALAPPDATA%\\Windrise\\Application 找浏览器。
- brand-src 的大图母版换成 c3：windrise-bridge-icon-1024.png 和套件
  windrise-app-icon-1024.png 字节相同；make_icons.py --check 通过。
- 新增补丁 chromium_patches/chrome/browser/background/extensions/background_mode_manager.cc：
  RegisterPrefs 里 kBackgroundModeEnabled 默认 false，只改这一行。
- 统计保持关闭：kPostHogEndpoint 仍是空串。
"""

import hashlib
import re
import subprocess
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
PKG = HERE.parents[1]
BASE = "ce18291e3"
KIT = Path("D:/work/_dispatch/design/opus/logo/kit/png/windrise-app-icon-1024.png")
THEME = PKG / "chromium_files/products/browseros/chrome/app/theme/chromium"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def diff_vs_base(rel):
    return subprocess.run(
        ["git", "diff", BASE, "--", rel], cwd=PKG, capture_output=True, text=True, encoding="utf-8"
    ).stdout


class Hooks1(unittest.TestCase):
    def test_branding_names(self):
        for name in ("BRANDING.release", "BRANDING.debug"):
            text = (THEME / name).read_text(encoding="utf-8")
            self.assertRegex(text, r"(?m)^PRODUCT_FULLNAME=Windrise Browser\s*$", name)
            self.assertRegex(text, r"(?m)^PRODUCT_SHORTNAME=Windrise Browser\s*$", name)

    def test_product_display_name(self):
        text = (PKG / "bos_build/products/browseros/product.py").read_text(encoding="utf-8")
        self.assertRegex(text, r'display_name="Windrise Browser"')

    def test_install_and_data_paths_unchanged(self):
        for rel in (
            "chromium_patches/chrome/install_static/chromium_install_modes.h",
            "chromium_patches/chrome/common/chrome_paths.cc",
        ):
            self.assertEqual(diff_vs_base(rel), "", rel)

    def test_c3_icon_master(self):
        self.assertEqual(sha(HERE / "brand-src/windrise-bridge-icon-1024.png"), sha(KIT))
        r = subprocess.run([sys.executable, str(HERE / "make_icons.py"), "--check"], capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

    def test_background_mode_default_off(self):
        patch = PKG / "chromium_patches/chrome/browser/background/extensions/background_mode_manager.cc"
        self.assertTrue(patch.exists(), "补丁文件要有")
        text = patch.read_text(encoding="utf-8")
        self.assertIn("diff --git a/chrome/browser/background/extensions/background_mode_manager.cc", text)
        self.assertIn("-  registry->RegisterBooleanPref(prefs::kBackgroundModeEnabled, true);", text)
        self.assertIn("+  registry->RegisterBooleanPref(prefs::kBackgroundModeEnabled, false);", text)
        changed = [l for l in text.splitlines() if re.match(r"^[-+](?![-+])", l)]
        self.assertEqual(len(changed), 2, changed)

    def test_analytics_stays_off(self):
        text = (PKG / "chromium_patches/chrome/browser/browseros/metrics/browseros_metrics_service.cc").read_text(encoding="utf-8")
        self.assertIn('constexpr char kPostHogEndpoint[] = "";', text)


if __name__ == "__main__":
    unittest.main()
