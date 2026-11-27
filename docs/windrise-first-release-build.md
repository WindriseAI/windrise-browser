# Windrise Browser 158 首发原生钩子与编译准备说明

本文档记录 Windrise Browser 158 首发版本（基于 Chromium 158.0.8127.18）的原生钩子补丁清单、设计插槽合同、译文回填分析与完整补丁回放证据。

---

## 1. 原则与边界

1. **一次定好再编**：设计定稿、插槽清单凑齐并确认编译前提条件后，在目标 Chromium 源码树（`<CHROMIUM_SRC>`）已有输出目录（`<OUT_DIR>`）增量编译一次。**本阶段只备可回放补丁与核对脚本，不开编、不发布**。
2. **原生只编通用容器与受限插槽**：C++ 只实现通用容器、布局约束、受限 API 身份隔离与必须改源码的固定项。具体业务（闪办广场、操作录制、AI 额度、省时间胶囊、审核卡等）均由随包内置扩展实现，改动内容或样式无需重编 C++。
3. **改名必须带译文 ID 迁移**：使用 GRIT 从 clean upstream 历史英文与消息名生成候选旧 ID，通过 `xtb_rebrand.py` 回填至新 ID，保留占位符结构。翻译缺失显式报错，严禁静默回退英文或删除中文。
4. **已有能力不重复移植**：`--disable-browseros-features` 开关与 Windows `drawAttention`（`windows_api.cc` -> `FlashFrame`）在 158 真实源码中已存在，直接复用。
5. **输入穿透遵循原生规则**：穿透光效层在 Views 树上显式禁用输入与焦点；交互卡片浮层仅卡片有效区域接鼠标，透明区域穿透到下方网页，不使用 `IgnoreInputEvents` 冻结网页。
6. **视觉必须比图签字**：界面件标注 `ui: true`，最终视觉效果以独立验证机真机截图比图为准；源码补丁回放通过不等于视觉通过。

---

## 2. 原生补丁范围与插槽规范

### 2.1 钩子与通用插槽分类表

| 钩子 | 分类 | 核心入口 / 关键源文件 | 重编范围 `[INFERENCE]` | 插件责任与合同 | 验收状态 |
|---|---|---|---|---|---|
| **1 品牌名** | C++固定项与资源 | `BRANDING.release` / `chromium_strings.grd` / `settings_chromium_strings.grdp` | 资源重新生成与链接 | 页面统一显示 Windrise Browser | 补丁已备，编译未测 |
| **2 关窗即退出** | C++固定项 | `chrome/browser/background/extensions/background_mode_manager.cc` | 局部编译并链接 | 主程序统一生命周期 | 源码已核，行为未测 |
| **3 浏览器独立图标（B6 定稿）** | C++固定项资源 | `tools/windrise/make_icons.py` / `brand-src/` / `resources/browseros/icons/win/chromium.ico` 及各尺寸原图 | 资源重新生成与链接 | 页面/工具栏共用 B6 套件，风桥隧道仍用 kit-v5 | 资源已接入并解码核验，视觉未测 |
| **4 左下角插槽** | C++通用插槽 | `browser_view.cc` / `browser_view_layout_impl.cc` / `vertical_tab_strip_region_view.cc` | UI/扩展API局部编译并链接 | 插件页面报高度，限幅 44~480px；AI 图标/额度/入口由插件绘制 | 补丁回放通过，运行未测 |
| **5 设置页主题变量** | C++通用插槽 | `web_ui_data_source_impl.cc` / `web_ui_util.cc` / `browseros_prefs.cc` | 共用WebUI局部编译并链接 | 读运行时偏好注入 `--wr-*` 变量；配置缺失退原生样式 | 补丁回放通过，运行未测 |
| **6 ColorMixer** | C++通用插槽 | `chrome_color_mixers.cc` / `browseros_prefs.cc` | 原生颜色/偏好局部编译并链接 | 读 profile 9个亮暗偏好键；改偏好触发重算或新窗口生效 | 补丁回放通过，运行未测 |
| **7 隐藏上游AI/新标签页** | C++固定入口/插件 | `browseros_extension_loader.cc` / `component_loader.cc` | 局部编译并链接 | 插件实现新标签页（搜索/常用网站/闪办/省时间/额度） | 补丁回放通过，运行未测 |
| **8 关独立更新** | C++固定项 | `winsparkle_glue.cc` / `version_updater_basic.cc` / `sparkle_glue.mm` | 更新模块局部编译并链接 | 禁用浏览器自身检查与安装，由主程序统一更新 | 补丁回放通过，运行未测 |
| **9 默认搜索Google（BR12）** | C++固定项 / 只读API | `template_url_prepopulate_data_resolver.cc` / `browser_os_api.cc/.h` / `browser_os.idl` | 搜索组件与扩展API局部编译并链接 | 首次兜底固定 Google；用户、扩展和策略优先级照旧；新标签页按当前默认引擎取名称、图标和 Chromium 展开的搜索地址 | 已补实现和回归用例，编译及真窗口待排队 |
| **10 首次导入页** | C++固定入口/插件 | `startup_browser_creator.cc` / `importer_list.cc` | 启动与导入接口局部编译并链接 | 插件引导页询问书签/密码导入，不替用户擅自同意 | 补丁回放通过，运行未测 |
| **11 折叠宽度与首字素** | C++通用插槽 | `tab_view.cc` / `vertical_tab_strip_region_view.cc` | 标签布局局部编译并链接 | 真实字体字素测量；中文首字/英文首字母大写/空标题取域名 | 补丁回放通过，运行未测 |
| **12 内部页统一风格** | C++插槽与字体 | `chrome_typography_provider.cc` / `neterror.css` | 字体与资源局部编译并链接 | HarmonyOS Sans SC 原样私有加载；安全警告页红底与文案不动 | 补丁回放通过，运行未测 |
| **13 工具栏5固定位** | C++通用插槽 | `pinned_toolbar_actions_model.cc` / `toolbar_view.cc` | 工具栏局部编译并链接 | 窄窗让位：320→最右槽逐个进拼图→主页/格子→200；空槽不占宽 | 补丁回放通过，运行未测 |
| **14 地址栏通用插槽** | C++通用插槽 | `browser_os_api.cc/.h` / `browser_actions.cc` / `browser_os.idl` | 扩展API/UI局部编译并链接 | 插件设置每标签文字/显隐/图标；仅受限固定内置扩展可用，定向回调 | 补丁回放通过，运行未测 |
| **15 统一办公上下分段** | C++通用插槽 | `tab_strip_view.cc` / `vertical_tab_strip_region_view.cc` | 标签布局局部编译并链接 | 双滚动容器与可拖分隔，AI区成员由插件指定，不改TabStripModel | 补丁回放通过，运行未测 |
| **16 组合页银边** | C++通用插槽 | `contents_container_outline.cc` / `contents_container_view.cc` | 边框视图局部编译并链接 | 静态银边走 Outline，流动流光共用光效层 | 补丁回放通过，运行未测 |
| **17 品牌译文回填** | 语言包迁移 | `xtb_rebrand.py` / `xtb_coverage.py` / 3 类 `.xtb` | 只重生成 `.pak`，不重编C++ | GRIT解析活动消息，老ID回填新ID，源码覆盖 81 语言，安装包随包携带全部语言包（`locales\*.pak`） | 脚本测试通过，编译未测 |
| **新增：窗口浮层插槽** | C++通用插槽 | `browser_view.cc` / `browser_view_layout_impl.cc` | 窗口级UI编译并链接 | 透明 WebView 只载内置插件，交互卡片命中有效，空白穿透不冻网页 | 补丁回放通过，运行未测 |
| **新增：穿透光效层** | C++通用插槽 | `browser_view.cc` / `contents_container_view.cc` | 窗口级UI编译并链接 | 原生树禁输入/焦点，网页区/分屏 bounds 变化事件驱动，插件画银光 | 补丁回放通过，运行未测 |
| **新增：Windows闪烁** | 只改插件 | `windows_api.cc:1223` -> `FlashFrame` | 无需重编 | 插件调用 `chrome.windows.update(windowId, {drawAttention: true})` | 源码已核，行为未测 |
| **上游：禁用开关** | C++固定项 | `command_line_prefs.cc/.h` / `.features.yaml` | 无需新增重编 | `--disable-browseros-features` 命令行高于用户低于策略且不落盘 | 源码已核，行为未测 |
| **上游：注意力API** | C++固定项 | `browser_os_api.cc` / `browser_os_attention_mac.mm` / `browser_os.idl` | 扩展API局部编译并链接 | 仅移植 3b48c65d7 必要片段，mac 调新能力，Windows 走 drawAttention | 补丁回放通过，运行未测 |

---

### 2.2 钩子 3 图标资源：来源、尺寸映射与验证边界

**来源**：图标采用 B6 定稿稿。仓内品牌源 `packages/browseros/tools/windrise/brand-src/` 只收 B6 实际用到的成品，字节与定稿一致，构建全程离线且只读仓内目录：

| 仓内品牌源 | 内容 |
|---|---|
| `windrise-browser-icon-{16,24,32,48,64,128,256,512,1024}.png` | 带乳白玻璃底的应用图标，每个尺寸单独绘制 |
| `windrise-browser-mark-white-{16,24,32,48,256}.png` | 深底用的无底白色图形标；定稿的 1024 白标与墨标当前没有消费者槽位，故未入库 |
| `windrise-browser-icon-256.svg` | 矢量母版；按定稿 README，16/24/32/48 的加粗矢量不得放大，只入库 64px 起可放大的 256 那份 |

`windrise-bridge-*`（kit-v5）母版原样保留给风桥隧道，`make_icons.py` 已不再读取它们；隧道、安装器与其他产品的图标不动。

**尺寸映射**（由 `make_icons.py` 落地，保留 `--brand-dir` / `--out` / `--check` 合同，仅依赖 Pillow；无独立定稿的尺寸一律不拿 1024 大图硬缩）：

| 输出尺寸 | 取源 | 规则 |
|---|---|---|
| 16 / 24 / 32 / 48 / 64 / 128 / 256 / 512 / 1024 | 同尺寸定稿 PNG | 字节级直用，不重采样 |
| 22 → `icon-24`；44 → `icon-48` | 加粗族 16–48 | ≤48 取对数距离最近的定稿帧 |
| 96 → `icon-128`；192 → `icon-256` | 标准族 | >48 取不小于目标的最小定稿帧，只缩不放 |
| 深底槽位 22/44 → `mark-white-24/48`；70/150 → `mark-white-256` | 白色图形标族 | 同上；`*_white`、`product_logo_22_mono`、`win/tiles/*` 按定稿 README 用不带底板的白标 |
| `win/chromium.ico`、`app_list.ico` | 定稿 ICO 帧集 16/24/32/48/64/128/256 | 仓内重组 PNG-in-ICO，每帧就是定稿原帧 |
| `win/incognito.ico` | `brand-src/windrise-browser-incognito-*`（16/24/32/48/64/128/256） | 每帧 = 同尺寸 B6 定稿 + 右下墨色无痕徽（白帽子 + 眼镜），`make_icons.py --render-incognito-finals` 8 倍超采样画出后入库；任务栏跳转列表「新建无痕窗口」与普通图标一眼分开（U26，11-22） |
| `mac/app.icns`、`AppIcon.icns`、`document.icns` | 定稿 ICNS 元素集 ic07/ic08/ic09/ic10/ic11/ic12/ic13/ic14（128/256/512/1024/32/64/256/512） | 仓内重组 PNG 元素，每元素就是定稿原帧 |
| `win/chromium_doc.ico`、`chromium_pdf.ico` | `mark-white-*`（字形 10/20/30/159）+ 既有蓝色底板合成 | 定稿未出文档图标，底板配色待设计确认 |

**实际验证边界**：只运行了 `python packages/browseros/tools/windrise/make_icons.py --check`（通过：58 个 `resources/browseros/icons` 文件与 12 个 agent/repo 资源和 brand-src 成品一致）与 `python -m py_compile` 语法检查，并逐个解码产出核对——`product_logo_16/24/256.png`、`win/chromium.ico`（7 帧）、`mac/app.icns`（8 元素）与定稿 `windrise-browser.ico` / `.icns` 逐帧像素全等。**浏览器本体未编译、未运行**：任务栏与桌面实际观感、深浅底色下的辨识度、DPI 缩放与 16px 可读性均为【未测】；`mac/Assets.car` 仍需 macOS `actool` 从本脚本更新后的 `Assets.xcassets` PNG 另行重编。

---

## 3. 5按钮窄窗让位规则

工具栏从左至右布局：后退、前进、刷新、主页、地址栏、5个固定扩展槽、拼图按钮、格子按钮、菜单。每个固定位 32px 宽，间距 4px。

窗口宽度缩窄时执行以下渐进收起规则：
1. **地址栏优先压缩**：地址栏从默认宽度缩至最小 320px。
2. **固定位向拼图溢出**：地址栏到达 320px 后，5 个固定槽从最右侧（第 5 槽）开始逐个收进拼图按钮面板中，直至 5 个固定位全部收起。被收起的项在拼图面板中排在最前列，图钉与启用状态保持不变。
3. **主页与格子收起**：窗口继续缩窄时，主页按钮收起，格子按钮收进主菜单。
4. **常驻保底与极限压缩**：后退、前进、刷新、地址栏、拼图按钮、菜单永远保持可见；地址栏进一步极限缩窄至 200px。
5. **反向恢复**：窗口变宽时按严格相反顺序依次展开。
6. **空槽规则**：插件或用户未固定满 5 个槽位时，空槽不占用任何宽度。

---

## 4. 钩子 17 译文迁移与 GRIT 覆盖证据

### 4.1 迁移机制
- **实现文件**：`packages/browseros/tools/windrise/xtb_rebrand.py`
- **函数签名**：`rebrand_translations(xtbText, messages, replacements) -> {xml, missing, translated}`
- **核心逻辑**：
  1. 输入校验：严格校验 XML 现有条目及输入 `messages`，若存在重复 ID 立即抛出 `ValueError`。
  2. 优先级：现有新 ID 译文绝对优先，重复执行保证幂等性。
  3. 结构保留：深拷贝 `<translation>` 节点，`<ph name="..."/>` 占位符结构、属性、内部文本及尾随 `tail` 均无损保留。
  4. 字符串替换：按照替换词长度降序替换（如先换 `Windrise BrowserOS` 再换 `Windrise Browser`），防止子串误伤。
  5. 明确缺译：无旧译文的消息列入 `missing` 列表，绝不伪造英文假译文。

### 4.2 单元验收测试验证
运行测试命令：
```bash
python packages/browseros/tools/windrise/test_xtb_rebrand.py
```
**运行结果**：
```text
......
----------------------------------------------------------------------
Ran 6 tests in 0.002s

OK
```
6 个行为测试全部通过：
- `test_old_id_backfill_is_real_translation_and_new_id`: 真实旧 ID 译文回填至新 ID。
- `test_placeholders_keep_structure_name_and_tail`: 占位符结构、name 属性与 tail 尾随文本精确保留。
- `test_existing_new_id_wins_and_repeated_run_is_idempotent`: 新 ID 优先，重复运行幂等。
- `test_missing_translation_is_explicit_and_not_english_fallback`: 缺译显式列出，不填充英文回退。
- `test_duplicate_ids_rejected_before_producing_output`: 重复 ID 严格拒绝。
- `test_unrelated_entries_and_chromeos_are_not_blindly_rebranded`: ChromeOS 项及非目标条目不被误替换。

### 4.3 81 语言与三资源覆盖分析（安装包随包携带全部语言）
运行分析工具：
```bash
python packages/browseros/tools/windrise/xtb_coverage.py --chromium-src <CHROMIUM_SRC>
```
- **资源覆盖**：
  - `chromium_strings` (`chrome/app/chromium_strings.grd`): 630 条含品牌活动消息。
  - `generated_resources` (`chrome/app/generated_resources.grd`): 7 条含品牌活动消息。
  - `components_chromium_strings` (`components/components_chromium_strings.grd`): 17 条含品牌活动消息。
  - **三类资源合计活动品牌消息**：654 条。
- **语言覆盖统计（81 种官方语言）**：
  - `zh-CN`: 总计 654 条，已翻译 24 条，成功回填恢复 578 条，可恢复率 **92.05%**。
  - `zh-TW`: 总计 654 条，已翻译 24 条，成功回填恢复 578 条，可恢复率 **92.05%**。
  - `en-GB`: 总计 654 条，已翻译 24 条，成功回填恢复 578 条，可恢复率 **92.05%**。
  - `ja`: 总计 654 条，已翻译 24 条，成功回填恢复 578 条，可恢复率 **92.05%**。
  - `fr`: 总计 654 条，已翻译 24 条，成功回填恢复 578 条，可恢复率 **92.05%**。
  - `de`: 总计 654 条，已翻译 24 条，成功回填恢复 544 条，可恢复率 **86.85%**。
  - 全球 81 种语言共计 52,320 处检查条目。
- **Windows 首发打包范围**：按 R729，Windows 安装包 `chrome.release` 随包携带全部语言包（`locales\*.pak`，实测 55 个 `.pak`）；界面语言默认跟随系统语言，并支持在风桥设置中自定义语言。
- **zh-CN 52 条未恢复项原因分类**：
  - **品牌专有名词**（Chromium 官方从未提供翻译，各语言原样保留英文）：如 `IDS_PRODUCT_NAME` ("Chromium")、`IDS_SHORT_PRODUCT_NAME`、`IDS_ACCNAME_APP`、`IDS_PASSWORD_MANAGER_TITLE_BRAND`。
  - **资源映射占位符**（Chromium 源码中明确标记为占位，无实际翻译）：如 `IDS_ABOUT_TERMS_OF_SERVICE` ("Not used in Chromium. Placeholder...")、`IDS_UPDATE_RECOMMENDED_DIALOG_TITLE_ALT`。
  - **158 上游实验性新增未译消息**：如 `IDS_PIN_INFOBAR_EXPERIMENTAL_*` 钉住工具栏实验提示，上游 Google 仅有英文提交，各语言 XTB 均无译文。
  - 完整 52 条逐条记录已落盘至 `packages/browseros/tools/windrise/fixtures/missing_translations_zh_cn.json`。

### 4.4 端到端（E2E）闭环验证
实际执行链路：
`真实 XTB 输入 -> xtb_rebrand.py 结构化迁移 -> 输出新 XTB 夹具 -> GRIT 原生 xtb_reader.Parse 解析验证`
- 验证代码：`generate_fixtures_and_e2e()` 内置于 `xtb_coverage.py`。
- 验证结果：`[E2E VERIFIED] Real XTB -> rebrand_translations -> migrated XTB -> GRIT xtb_reader OK!`。
- 夹具产物：
  - `packages/browseros/tools/windrise/fixtures/rebranded_sample_zh_cn.xtb`
  - `packages/browseros/tools/windrise/fixtures/sample_messages.grd`
  - `packages/browseros/tools/windrise/fixtures/missing_translations_zh_cn.json`
  - `packages/browseros/tools/windrise/fixtures/xtb_coverage_summary.json`

---

## 5. 补丁集合完整顺序回放证据

依据官方 patchkit 排序规则（`batch_apply.find_patch_files`），排除元数据和标记文件后，`packages/browseros/chromium_patches/` 下共有 **420 个补丁文件**。

### 5.1 隔离源文件集稀疏回放验证
运行工具：
```bash
python packages/browseros/tools/windrise/patch_replay.py --chromium-src <CHROMIUM_SRC> --mode full
```
**安全与隔离保障**：
- 工作目录为独立隔离临时目录，未在 `<CHROMIUM_SRC>` 执行任何 reset / checkout 操作。
- 通过 `git cat-file -p 6449d9f9:<path>` 从干净基线稀疏提取 277 个受影响源文件，避免复制整树。
- 记录每个输入文件的初始 SHA256。
- 按正式 patchkit 字母序严格依次应用 420 个补丁。

**运行结果**：
```text
============================================================
PATCH REPLAY SUMMARY (Mode: full)
============================================================
Total Patches:   420
Applied:         420
Failed:          0
Duration:        10.185s
Replay Success:  True
Saved replay report to: packages/browseros/tools/windrise/fixtures/patch_replay_report.json
```
**全量 420 个补丁 100% 成功应用，失败数 0**。回放报告落盘至 `packages/browseros/tools/windrise/fixtures/patch_replay_report.json`。

### 5.2 增量采样回放验证
运行工具：
```bash
python packages/browseros/tools/windrise/patch_replay.py --chromium-src <CHROMIUM_SRC> --mode incremental
```
**运行结果**：
```text
============================================================
PATCH REPLAY SUMMARY (Mode: incremental)
============================================================
Sample Verified: True
Duration:        0.043s
Saved replay report to: packages/browseros/tools/windrise/fixtures/patch_replay_incremental.json
```

---

## 6. 只读 Preflight 与受控编译入口

脚本：`packages/browseros/tools/windrise/build_preflight.py`

### 6.1 默认只读检查（Preflight）
运行命令：
```bash
python packages/browseros/tools/windrise/build_preflight.py
```
**检查项证据**：
- **物理内存**：可用 10.57 GiB，远高于门限 2.0 GiB（✓ PASS）。
- **CPU 余量**：空闲余量 >= 75%，高于门限 20%（✓ PASS）。
- **磁盘空间**：D 盘可用 36.46 GiB，高于门限 15.0 GiB（✓ PASS）。
- **Flowmap 与协作板保护**：本地端口 8765 处于活动监听状态，未被中断或停止（✓ PASS）。
- **源码与构建树**：`<CHROMIUM_SRC>` 与 `out/Default_browseros_x64` 存在且配置完整（✓ PASS）。
- **ThinLTO / PGO 优化保留**：`args.gn` 确认 `is_official_build = true`、`symbol_level = 0`，保留 ThinLTO / PGO（✓ PASS）。
- **工具链就绪**：检测到 `<DEPOT_TOOLS>/autoninja.bat`（✓ PASS）。

### 6.2 编译总闸门控验证（Gate Rejection）
若未满足全部四个前提条件传入 `--compile`：
```bash
python packages/browseros/tools/windrise/build_preflight.py --compile
```
**结果**：立即报错拒绝，退出码 2：
```text
ERROR: Compilation request REJECTED! Missing mandatory gates:
  ✗ design_finalized: Missing required signoff/flag
  ✗ slots_ready: Missing required signoff/flag
  ✗ user_approved: Missing required signoff/flag
  ✗ paused_dispatch: Missing required signoff/flag
Compilation cannot start until all four gates are satisfied.
```

### 6.3 Dry-Run 编译命令演练
当满足全部四项前置并在测试模式下运行：
```bash
python packages/browseros/tools/windrise/build_preflight.py --compile --design-finalized --slots-ready --user-approved --paused-dispatch --dry-run
```
**结果**：输出实际编译命令，退出码 0，但绝不执行真实编译：
```text
All four operational gates satisfied:
  ✓ design_finalized: Satisfied
  ✓ slots_ready: Satisfied
  ✓ user_approved: Satisfied
  ✓ paused_dispatch: Satisfied

[DRY RUN] Prepared incremental compilation command:
  autoninja.bat -C <OUT_DIR> chrome chromedriver

Dry-run verified: compile command prepared but NOT executed.
(Current task is patch preparation only, not compiling).
```

---

## 7. 待落实的验收矩阵（真实编译后核验）

所有在真实编译与真机运行前无法获得证据的行为，状态必须诚实标记为 **【未测】**：

| 范围 | 当前已就绪的证据 | 统一编译后真机待验清单（状态全为【未测】） |
|---|---|---|
| **17 译文** | 真实 GRIT 生成当前/旧 ID、占位符结构保留、三资源 81 语言缺译表、E2E 闭环测试 | **【未测】** 中文真实环境打开设置、菜单、出错页，验证无 Chromium 旧品牌及异常英文回退 |
| **4/14/浮层/光效** | 420 补丁完整顺序回放通过，API IDL 合同就绪 | **【未测】** 普通扩展调用受限接口被拒绝；跨 profile/隐身窗口隔离；卡片可点击且空白穿透；光效层无焦点；DPI 缩放与关窗清理 |
| **5/6/12 主题与字体** | WebUI 与 ColorMixer 入口补丁回放就绪 | **【未测】** 亮暗模式切换即时生效；系统字体失败回退；安全警告页红底醒目保留；第三方页面及 DevTools 样式不被破坏 |
| **11/13/15 标签与工具栏** | 5 按钮 320/200 窄窗让位规则规范冻结 | **【未测】** 窗口变窄时 5 个槽依次收进拼图面板，空槽不占宽；中文/emoji首字素折叠显示与悬停全标题；双段滚动拖拽持久化 |
| **3/7 内置资源与插件** | 随包内置插件加载逻辑补丁就绪；钩子 3 的 B6 图标资源已过 `make_icons.py --check` 与逐帧解码核验（见 2.2） | **【未测】** 随包自动载入、用户无法卸载；新标签页各业务卡片正常渲染；更新跟随主程序；桌面/任务栏/快捷方式取到 B6 图标且与风桥隧道靠形状可辨，深浅底与 16px 无糊 |
| **8/9/10 固定项** | 更新关闭与搜索入口补丁就绪 | **【未测】** 无独立更新网络请求；首次启动默认 Google 搜索且保留用户后续改动；首次引导页未经同意不导入数据 |
| **1/2/Windows闪烁/上游项** | 关窗即退出与 FlashFrame 源码路径核对 | **【未测】** 关窗进程完全退出；任务栏闪烁能正常触发并适时停止；命令行参数强制值高于用户低于策略且不落盘 |

---

## 8. 回滚方案

若需回滚本批工作：
1. 撤销目标发布准备分支上的提交：
   ```bash
   git revert HEAD
   ```
2. 构建机 `<CHROMIUM_SRC>`、编译产物 `out/` 及用户 profile 保持纯只读，未做任何就地修改，无需还原。

## 9. 风桥助手扩展保护（ext-lock-1113）

基线为 `edge-light-layer-1113b` 的 `f4005acda`。助手保持 `kCommandLine` 加载，固定 ID 为 `amlnokjigkbgngdgkofikfphjbknfodp`。专用管理策略只对固定公钥且资源签名完整的助手实施 `MustRemainInstalled`、`MustRemainEnabled` 和用户/扩展设置修改限制，不设置企业管理策略，不改变其他扩展。

首次校验包含固定公钥、清单 RSA-SHA256 签名和完整目录文件的 SHA256。校验前先启动递归文件监听；任何变更或监听错误使该 Extension 实例永久失去可信状态，重新加载才能重新验签。管理查询、光效活动接口和每帧 layer 可见性查询只读取已验证状态，不反复读取字体等资源。未验证的同 ID 扩展不能加载，其他扩展不能调用风桥专有接口，layer 也不显示来自失效助手的资源。同步首验仅在固定身份加载边界执行，`base/threading/thread_restrictions.h` 仅给该验证函数增加窄范围许可。

公钥固定在浏览器，扩展 JS、清单及签名随主程序更新；使用同一私钥重新签名即可，无需重编浏览器。主程序先修复账号资料目录的 `WindriseAssistant/`，确认 `management.getSelf().mayDisable === false` 后才完成握手，并监测运行时失效状态。已保留的用户禁用原因由 registrar 的 `MustRemainEnabled` 分支清除。

8 个改动补丁已在任务临时目录中打到 `158.0.8127.41` 原文并逐字节比较成功；没有修改源码树或执行编译。新增 `ExtensionRegistrarTest.MustRemainEnabledOverridesPersistedUserDisableOnInstall`，覆盖既有用户禁用原因在受保护安装时被清除的决策，以及普通扩展仍保留该原因；此原生回归待编译操作员执行。管理页按钮、图标菜单、旧禁用资料目录、损坏恢复、换签名资源无需重编和全局无新增管理提示，仍须按 `请求-05-扩展保护.md` 编译并实机验收。

