# 编译机增量重新应用流程

适用于已经完成过一次完整构建、并且 Chromium 源码里已经应用过上一版 BrowserOS 补丁的编译机。

- BrowserOS 仓库：`C:\src\BrowserOS`
- Chromium 源码：`C:\src\chromium\src`
- 脚本：`packages\browseros\tools\windrise\apply.ps1`

## 为什么不能重跑完整 prep

`clean`、`download_resources`、`series_patches` 都假设 Chromium 是干净的 fixed revision。
在这台机器上重跑它们会清掉已应用补丁或重新下载资源，导致构建失败。这个脚本只做增量部分：

1. 从 `git diff` 找出指定提交区间里变化的补丁文件；
2. 对每个补丁先恢复它目标文件的原始状态（新文件则删除）；
3. `git apply --recount` 重新应用该补丁；
4. 重新执行 `chromium_replace`、`string_replaces`、`resources,bundled_extensions`；
5. 打印 `autoninja`，不执行。

## 用法

在 BrowserOS 仓库根目录或任意位置执行：

```powershell
cd C:\src\BrowserOS\packages\browseros
powershell -ExecutionPolicy Bypass -File .\tools\windrise\apply.ps1 `
  -From b8de97a6e `
  -To HEAD
```

默认值：

| 参数 | 默认值 |
| --- | --- |
| `-BrowserosRoot` | 脚本所在目录的 `..\..`（即 `packages\browseros`） |
| `-ChromiumSrc` | `C:\src\chromium\src` |
| `-Product` | `browseros` |
| `-BuildType` | `release` |
| `-Arch` | `x64` |
| `-OutDir` | `out/Default_<product>_<arch>` |
| `-WindriseExtensionDir` | 环境变量 `WINDRISE_EXTENSION_DIR`（若未传则按常见同机路径探测，找不到即阻断） |

应用补丁前，`apply.ps1` 会先对 `browseros` 的扩展目录执行 RSA-SHA256 验签和全文件 SHA-256 核对。资源准备完成后，脚本将通过校验的扩展写入 `out/<dir>/windrise_extensions/windrise-extension`。Windows 构建和打包也会验证扩展目录，打包步骤还会在输出制品前验证安装器暂存目录。

```powershell
.\tools\windrise\apply.ps1 `
  -From b8de97a6e -To HEAD `
  -WindriseExtensionDir C:\src\bridge-replica\electron\windrise-extension
```

如果只回放当前分支最后几个提交：

```powershell
.\tools\windrise\apply.ps1 -From HEAD~3 -To HEAD
```

## 脚本做了什么

### 1. 找变化的补丁

```powershell
git -C $BrowserosRoot diff --name-status --relative $From $To -- chromium_patches
```

跳过 `.features.yaml`、`.store.yaml`、隐藏文件以及
`.deleted` / `.binary` / `.rename` 标记文件。

### 2. 恢复目标文件

补丁里的目标路径就是 `diff --git a/... b/...` 中的 `b/...`，与
`chromium_patches` 下的相对路径一致。

- 普通修改：`git -C $ChromiumSrc restore --source=HEAD --worktree -- <path>`
- `new file mode` / `--- /dev/null`：删除 `$ChromiumSrc/<path>`
- 补丁本身被删除：目标文件能回到 upstream 就从 `HEAD` 恢复，只在 BrowserOS
  中存在的新文件则删除。

### 3. 重新应用

```powershell
git -C $ChromiumSrc apply --recount --whitespace=nowarn -p1 <patch>
```

`--recount` 让 git 按 hunk 实际内容重算行数，因此补丁文件即使手改过
`@@` 行也能应用。

### 4. 重新应用覆盖文件和字符串

```powershell
uv run browseros build --modules chromium_replace `
  --product browseros --build-type release --arch x64 `
  --chromium-src C:\src\chromium\src
```

字符串替换前先恢复：

```powershell
git -C C:\src\chromium\src checkout -- chrome/app/chromium_strings.grd
git -C C:\src\chromium\src checkout -- chrome/app/settings_chromium_strings.grdp
uv run browseros build --modules string_replaces ...
```

资源：

```powershell
uv run browseros build --modules resources,bundled_extensions ...
```

### 5. 编译

脚本最后会打印类似：

```text
autoninja.bat -C out/Default_browseros_x64 chrome chromedriver
```

需要真正编译时手动执行。Linux 上还需要加 `chrome_sandbox`。

## 注意

- **不要**运行 `clean`、`download_resources`、`series_patches`。
- 只改补丁文件的内容时，也要先跑
  `python tools\windrise\recount_patches.py`（或 `--check` 确认），否则
  下一次 `git apply` 会报 `corrupt patch`。
- 如果改的是 `chromium_files` 覆盖文件，不需要重新 apply 补丁，只跑
  `--modules chromium_replace` 即可。
- 如果改的是 `chrome/app/chromium_strings.grd` /
  `settings_chromium_strings.grdp`，必须先 checkout 再跑
  `string_replaces`，否则会在已替换过的文本上再替换一遍。
