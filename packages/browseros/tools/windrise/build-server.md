# 从源码构建本地 server

编译机上的 `browseros_server` 是从官方安装版拷来的预编译二进制。它的配置入口只有
`--config <sidecar.json>`，sidecar 里主要读 `ports`、`directories`、`flags`。我们给
sidecar 增加了 `flags.host`，但预编译二进制不认识这个字段，仍然按它原先的绑定逻辑
（官方版本通常是 `0.0.0.0`）监听。要真正只监听 `127.0.0.1`，必须用源码重新构建。

## 构建命令

在 `C:\src\BrowserOS\packages\browseros-agent` 下：

```powershell
bun install
bun scripts\build\server.ts --target=windows-x64 --ci
```

`--ci` 的好处是不需要 R2 凭证，也不需要生产环境里的 PostHog / Sentry key；产物是：

```text
dist\prod\server\windows-x64\resources\bin\browseros_server.exe
dist\prod\server\windows-x64\resources\...
dist\prod\server\browseros-server-resources-windows-x64.zip
```

其他本机架构：

```powershell
bun scripts\build\server.ts --target=macos-arm64 --ci
bun scripts\build\server.ts --target=macos-x64 --ci
bun scripts\build\server.ts --target=linux-x64 --ci
```

构建脚本来自 `packages/browseros-agent/scripts/build/server.ts`，实际执行的是
`bun build --compile`（见 `packages/build-server-tools/src/compile.ts`）。

## 把新二进制放进构建

解压 zip 后，`resources/bin/` 里是 `browseros_server.exe`，其余 `resources/` 是
运行资源。把它替换到 BrowserOS 的 server 资源目录，再重新跑资源模块：

```powershell
$stage = "C:\src\BrowserOS\packages\browseros\resources\binaries\browseros_server\windows-x64"
Expand-Archive -Force `
  dist\prod\server\browseros-server-resources-windows-x64.zip `
  $stage
```

然后在 `packages\browseros` 下重新应用资源：

```powershell
uv run browseros build --modules resources,bundled_extensions `
  --product browseros --build-type release --arch x64 `
  --chromium-src C:\src\chromium\src
```

## host 传递约定

Chromium 的 `browseros_server_config.cc` 会写：

```json
{
  "flags": {
    "allow_remote_in_mcp": false,
    "host": "127.0.0.1"
  }
}
```

只有 `allow_remote_in_mcp=true` 时才写 `"host": "0.0.0.0"`。源码版 server 的
`apps/server/src/config.ts` 会读取 `flags.host`；旧预编译版会忽略它。因此：

- 用旧预编译二进制：只能通过防火墙/系统策略限制端口，不能靠 sidecar 改绑定地址；
- 用源码构建的二进制：默认 `127.0.0.1`，远程 MCP 开关打开时才 `0.0.0.0`。

## 10-30 补充：`--ci` 产物缺 `bin/third_party/bun.exe`，数据目录

- `--ci` 模式不从 R2 下载 vendor 资源，所以产物里没有 `resources/bin/third_party/bun.exe`
  （server 跑 ACP agent 时用的 bun，见 `apps/server/src/lib/agents/host-acp/bundled-bun.ts`）。
  替换编译机上的 server 时只换 `resources/bin/browseros_server.exe`，`third_party/` 和
  `db/migrations/` 保留原来的（迁移 SQL 与源码一致，只差 CRLF）。
- 10-30 在 Chromium 侧（`process_controller_impl.cc`）给 server 进程设置
  `BROWSEROS_DIR=<User Data>\.browseros`。不设时 server 用 `~/.browseros` 存
  SQLite / 会话 / installation.json，和本机装的 BrowserOS 共用；那份库损坏
  （`SQLITE_CORRUPT`）时 Windrise 的 server 一启动就退出。
- 手动排查 server 起不来：浏览器不收集 server 的 stdout/stderr，启动阶段的报错不会进
  `browseros-server.log`。用一份改了 `execution` 和 `ports.server` 的 config，加上
  `BROWSEROS_DIR` 指向临时目录，前台运行 `browseros_server.exe --config=<json>` 看输出。
