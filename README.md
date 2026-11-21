# 风桥浏览器 / Windrise Browser

风桥浏览器（Windrise Browser）是基于 Chromium 158（基线 `158.0.8127.41` / `3479de84ee`）构建的开源桌面浏览器。

Windrise Browser is an open-source desktop browser built on Chromium 158 (baseline `158.0.8127.41` / `3479de84ee`).

## 开源许可 / License

- 本项目按 **GNU Affero General Public License v3.0 (AGPL-3.0)** 开源发布，完整许可证正文见根目录 [LICENSE](LICENSE)。
- 本程序按“现状”提供，**不提供任何明示或暗示的担保**（WITHOUT ANY WARRANTY），详见 AGPL-3.0 第 15–17 条。
- Chromium 及第三方开源组件的许可证与版权声明见软件内 `chrome://credits` 与 [LICENSE.ungoogled_chromium](LICENSE.ungoogled_chromium)。
- 内置界面字体 HarmonyOS Sans SC 许可协议见 `packages/browseros/resources/windrise/fonts/harmonyos_sans/LICENSE.txt`。

## 从源码构建 / Building from Source

### 环境要求 / Prerequisites

- Chromium 源码树（检出基线提交 `3479de84ee`，对应版本 `158.0.8127.41`）
- Python 3.12+（含 `uv`）
- 平台编译工具链（Windows Visual Studio Build Tools / macOS Xcode Command Line Tools / Linux `build-essential`）

### 构建步骤 / Steps

1. 准备 Chromium 基线源码并校验补丁完整可回放：
   ```bash
   python packages/browseros/tools/windrise/recount_patches.py --check --git
   ```
2. 应用 `packages/browseros/chromium_patches/` 下的补丁，并叠加 `packages/browseros/chromium_files/` 与 `packages/browseros/resources/` 中的覆盖文件与资源（也可使用 `packages/browseros/tools/windrise/patch_replay.py` 回放核对）。
3. 使用标准 Chromium GN / Ninja 流程编译 `chrome` 与平台安装包目标（Windows 为 `mini_installer`）。

## 官方网站与反馈 / Links

- 官网 / Website: https://www.windrisebridge.com/ （中文：https://www.windrisebridge.cn/）
- 源码与问题反馈 / Source & Issues: https://github.com/WindriseAI/windrise-browser
