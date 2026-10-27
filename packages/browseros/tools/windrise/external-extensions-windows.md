# Windows 按资料目录读取 External Extensions

## 问题

Windrise Bridge 的「插件一键同步到所有格子」按格子给每个 `--user-data-dir` 写：

```
<user-data-dir>/External Extensions/<extension_id>.json
{"external_update_url": "https://clients2.google.com/service/update2/crx"}
```

期望浏览器下次启动时自动安装。Chromium 里这条「资料目录下的 External Extensions」通路只在三个平台接上：

- `chrome/common/chrome_paths.cc`：`chrome::DIR_USER_EXTERNAL_EXTENSIONS` 的 `PathProvider` 分支被
  `#if BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_MAC) || (BUILDFLAG(IS_LINUX) && BUILDFLAG(CHROMIUM_BRANDING))` 圈住；
- `chrome/common/chrome_paths.h`：枚举成员 `DIR_USER_EXTERNAL_EXTENSIONS` 本身也在同样的 `#if` 里；
- `chrome/browser/extensions/external_provider_impl.cc`：注册该 provider 的代码同样带同样的平台门。

Windows 上因此既没有这个路径键，也没有对应的 provider，JSON 写进去没人读。Windows 只有注册表（`ExternalRegistryLoader`）与可执行文件旁的 `DIR_EXTERNAL_EXTENSIONS`，都不是按格子隔离的。

## 改了什么

只改 3 个已有补丁文件，没有新增补丁文件，也没有动 `.features.yaml`（`chrome_paths.*` 已由 `branding` 声明、`external_provider_impl.cc` 已由 `ota-updater` 声明，本改动落在同一批文件的既有 hunk 内）。

### 1. `chromium_patches/chrome/common/chrome_paths.h`

把 `DIR_USER_EXTERNAL_EXTENSIONS` 枚举成员的平台门加上 Windows：

```c
#if BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_WIN) || \
    (BUILDFLAG(IS_LINUX) && BUILDFLAG(CHROMIUM_BRANDING)) || BUILDFLAG(IS_MAC)
  DIR_USER_EXTERNAL_EXTENSIONS,  // ...
```

`build/build_config.h` 该头文件本来就有 include，`BUILDFLAG(IS_WIN)` 可直接用。其他平台的预处理结果不变。

### 2. `chromium_patches/chrome/common/chrome_paths.cc`

在 `PathProvider` 的 `switch (key)` 里补上 Windows 分支，落在该文件已有的 BrowserOS 新增块尾部（`default:` 之前）：

```c
#if BUILDFLAG(IS_WIN)
    case chrome::DIR_USER_EXTERNAL_EXTENSIONS: {
      if (!base::PathService::Get(chrome::DIR_USER_DATA, &cur)) {
        return false;
      }
      cur = cur.Append(FILE_PATH_LITERAL("External Extensions"));
      break;
    }
#endif  // BUILDFLAG(IS_WIN)
```

与上游 macOS/Linux 分支逐字同构：解析成 `DIR_USER_DATA` 下的 `External Extensions`，且**不设** `create_dir`，所以取这个路径不会创建目录（目录由写 JSON 的桥接端负责）。运行时 `DIR_USER_DATA` 已被 `--user-data-dir` 覆盖，所以每个格子解析到自己的目录。

### 3. `chromium_patches/chrome/browser/extensions/external_provider_impl.cc`

`ExternalProviderImpl::CreateExternalProviders` 末尾（BrowserOS 自带 loader 之后）追加：

```c
#if BUILDFLAG(IS_WIN)
  if (!profile->GetPrefs()->GetBoolean(pref_names::kBlockExternalExtensions)) {
    auto user_external_extensions_provider =
        std::make_unique<ExternalProviderImpl>(
            service,
            base::MakeRefCounted<ExternalPrefLoader>(
                chrome::DIR_USER_EXTERNAL_EXTENSIONS, ExternalPrefLoader::NONE,
                nullptr),
            profile, ManifestLocation::kExternalPref,
            ManifestLocation::kExternalPrefDownload, Extension::NO_FLAGS);
    provider_list->push_back(std::move(user_external_extensions_provider));
  }
#endif  // BUILDFLAG(IS_WIN)
```

这一行 `ExternalProviderImpl(..., kExternalPref, kExternalPrefDownload, Extension::NO_FLAGS)` 与上游 macOS/Linux 的 per-user provider 完全相同；没有 `set_auto_acknowledge`、没有 `set_install_immediately`、没有 `WAS_INSTALLED_BY_DEFAULT`。所需头文件（`external_pref_loader.h`、`chrome_paths.h`、`extensions/common/extension.h`）上游本就 include，未加新依赖。

## 为什么安全

- **只读本 profile 的 user-data-dir。** 路径就是 `DIR_USER_DATA` 加一个固定子目录，不读系统全局路径、不读注册表、不跨 profile、不联网（`external_update_url` 的下载仍走 Chromium 原生 crx 安装与更新流程）。
- **id 与内容校验全部沿用原生 loader/provider**，没有任何放宽：
  - `ExternalPrefLoader` 只扫该目录第一层的 `*.json`（跳过 `external_extensions.json`），文件名去掉后缀即扩展 id；
  - `ExternalProviderImpl::RetrieveExtensionsFromPrefs` 用 `crx_file::id_util::IdIsValid` 丢弃非法 id，要求 `external_crx`/`external_version` 与 `external_update_url` 恰好出现一组，并校验 update URL 合法性；
  - `ExternalPrefLoader::NONE` 表示不做「路径必须由管理员控制」的 macOS 检查——与上游 per-user 来源一致（那是系统级来源才需要的）。
- **非策略来源，可正常卸载。** location 走外部 prefs 系（`crx_location` = `kExternalPref`，从 update URL 安装后落在 `kExternalPrefDownload`），不是 `kExternalPolicy*`，所以在 `chrome://extensions` 里是可移除的外部安装扩展，用户对它的修改权限不受管理策略限制。
- **卸载后不会被重装。** 用户移除时 `ExtensionPrefs::OnExtensionUninstalled` 会把 id 写进 `extensions.external_uninstalls`（条件是 `!external_uninstall && Manifest::IsExternalLocation(location)`，我们的 location 满足），下次启动 provider 依旧上报该 id，但 `PendingExtensionManager::AddFromExternalUpdateUrl` 在「尚未安装」分支里对非策略来源命中该 id 就 `return false`，安装不会发生。这是 Chromium 原生行为，不是我们新写的去重。
- **优先级不越权。** 该来源的 `crx_location` 是 `kExternalPref`，从 update URL 安装后扩展落在 `kExternalPrefDownload`。按 `Manifest::GetHigherPriorityLocation` 的 rank（internal 0 < externalPrefDownload 1 < externalPref 2 < externalRegistry 3 < unpacked 4 < commandLine 5 < externalPolicyDownload 6 < externalPolicy 7 < externalComponent 8 < component 9），只有用户手动安装的 `kInternal` 比它低；注册表来源（Windows 上原有通路）、解包加载、命令行、策略强制安装、BrowserOS 自带扩展（`kExternalComponent`，rank 8）全部高于它。因此这个新来源既不会顶掉策略/自带扩展，也不会覆盖已有的注册表安装。

  具体到同 id 冲突：注册表以 `path`/`version` 声明时扩展装成 `kExternalRegistry`（rank 3），高于本目录的 rank 1，注册表胜出且不会被降级；注册表与 `external_update_url` 形式共用 `kExternalPrefDownload` 作为 download location，两者落在同一 rank，不会互相覆盖成更高优先级。

- **受既有策略开关约束。** 该 provider 与注册表来源一样被 `extensions.block_external_extensions` 前置判断包住，企业禁用外部扩展时不会绕过。
- **没有开新攻击面。** 能写 `<user-data-dir>/External Extensions/*.json` 的进程，本来就能直接改该 profile 的 `Preferences`/`Local State`、投放任意扩展，权限等级相同。副作用是把「用户自己目录里的 JSON」从「Windows 上被忽略」变成「会安装」，这与 macOS/Linux 长期行为一致。

## 行为细节

- 首次安装会走 Chromium 的外部安装确认流程：扩展被安装但处于 `DISABLE_EXTERNAL_EXTENSION`（提示条/菜单里告知「由其他程序添加」），用户确认后启用。这是上游 `auto_acknowledge=false` 的原生语义。若「一键同步」希望静默启用，只需在该 provider 上加一行 `->set_auto_acknowledge(true)`；卸载与 blocklist 行为不受影响（blocklist 判断与 `mark_acknowledged` 无关）。本补丁故意保留原生确认流程，没有替用户做这个决定。
- 目录不存在时 `ExternalPrefLoader` 直接返回空集合，不报错、不创建目录，所以没开同步的格子不受影响。
- 只影响 Windows 构建；macOS/Linux/ChromeOS 的预处理输出与改动前一致。

## 编译机上如何验证

前提：能构建 Windows 版 Chromium（补丁栈按正常流程应用，本仓库的 `bos_build` / `bpatch` 都会带上这 3 个文件）。

1. 构建产物就绪后，先用一个干净的格子目录启动一次再退出，让 `user-data-dir` 初始化：

   ```cmd
   out\Release\chrome.exe --user-data-dir=D:\tmp\grid-test
   ```

2. 写一个外部扩展示意文件（id 用文件名，必须是合法 32 字符扩展 id；`external_update_url` 指向商店更新地址）：

   ```cmd
   mkdir "D:\tmp\grid-test\External Extensions"
   echo {"external_update_url":"https://clients2.google.com/service/update2/crx"} > "D:\tmp\grid-test\External Extensions\ghbmnnjooekpmoecnnnilnnbdlolhkhi.json"
   ```

   （该 id 是 Google Docs Offline，任意商店扩展都可以。）

3. 再次启动：

   ```cmd
   out\Release\chrome.exe --user-data-dir=D:\tmp\grid-test
   ```

4. 期望结果：`chrome://extensions` 里出现该扩展（外部安装，带「由其他程序添加」的提示；未确认前是关闭状态）。启用/关闭都由用户操作。

5. 卸载持久性：在 `chrome://extensions` 里移除该扩展，退出，再次启动。期望：不会被重新安装；`D:\tmp\grid-test\Default\Preferences` 的 `extensions.external_uninstalls` 数组里出现该 id。若把 `External Extensions` 目录整个删掉再删一次，Chrome 会把该 id 从 blocklist 里清掉（原生行为）。

6. 负例（都应无安装、无崩溃）：
   - 文件名不是合法 id（如 `abc.json`）→ `chrome_debug.log` 里 `is not a valid id`；
   - JSON 内容非法或缺 `external_update_url` → `Malformed extension dictionary`；
   - 目录不存在 → 静默跳过。

7. 回归项：`--user-data-dir` 换成两个不同目录，各自只放一个不同的 JSON，确认互相隔离（这是「格子」模型的关键）。

## 本机可做的校验（无 Chromium 源码、无法编译）

1. **hunk 行数**：本目录脚本

   ```bash
   python packages/browseros/tools/windrise/recount_patches.py --check --strict \
     packages/browseros/chromium_patches/chrome/common/chrome_paths.h \
     packages/browseros/chromium_patches/chrome/common/chrome_paths.cc \
     packages/browseros/chromium_patches/chrome/browser/extensions/external_provider_impl.cc
   ```

   逐 hunk 核对 `@@ -a,b +c,d @@` 与实际 body（context/减/加）数量，并检查 hunk 顺序、`---/+++` 与实际路径一致。全树 `--check`（不带路径）会把仓库里既有的 stale 计数当 warning 打印、结构错误当 error。

2. **补丁能打到纯净树上**：以 `packages/browseros/BASE_COMMIT`（`16c3e554…`）从上游取这三份文件的原始版本，对改动后的补丁执行 `git apply --check` 与正式 `git apply`，结果与编辑后的目标文件逐字节相同。这一步已经跑过（见提交说明），比手工数行更硬。

3. 编译期无法在本机验证的部分：Windows 下 `ExternalPrefLoader`/`ExternalProviderImpl` 的实参类型、`pref_names::kBlockExternalExtensions` 的可见性，均与同函数内上游 macOS/Linux 分支逐字同构，风险集中在「平台门是否都补齐」，而这一点已由 `chrome_paths.h` 的枚举改动覆盖。

## 已知不确定点

- **未能本机编译**，`chrome_paths.h` 枚举门与 `chrome_paths.cc` 的 `case` 之间的一致性靠人工核对：Windows 下上游已有的 `DIR_USER_EXTERNAL_EXTENSIONS` `case` 被 `#if` 排除，新增的 `case` 又只在 Windows 生效，因此不存在重复 label。这一条只有编译器能最终确认。
- **`--user-data-dir` 的路径键缓存时机**：本改动依赖 `DIR_USER_DATA` 在 provider 创建前已被命令行覆盖（上游 macOS/Linux 同路径假设）。没有在 Windows 上实测「不传 `--user-data-dir` 的默认 profile」与「传了的格子」在同一进程内切换的场景（Chromium 本身也不支持同一进程多 user-data-dir）。
- **发现但与本次无关的既有问题**：`chromium_patches` 里有 8 个文件的 13 个 hunk 的 `@@` 计数与实际 body 不一致（例如 `chrome/browser/ui/browser_actions.cc` 第一个 hunk 声明 `+21,7` 实际 10 行），`git apply` 会直接报 `corrupt patch`，只能靠 `--recount` 类工具应用。本次未修改这些文件（属于他人改动范围），仅由 `recount_patches.py` 以 warning 形式记录。
