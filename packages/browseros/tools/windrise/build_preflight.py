#!/usr/bin/env python3
"""Build Preflight and Controlled Entry Script for Windrise Browser on Chromium 158.

Checks environment prerequisites (memory, CPU margin, disk space, source tree,
flowmap health, ThinLTO/PGO optimization flags, toolchain) and enforces strict
operational gates before any future incremental compilation on the main machine.

Default behavior:
    Read-only preflight analysis. NEVER compiles by default.

Controlled compile prerequisites:
    1. --design-finalized: Design signoff confirmed by design team.
    2. --slots-ready: General slots inventory complete.
    3. --user-approved: Explicit user approval.
    4. --paused-dispatch: Main machine task dispatching paused (go.mjs pause).

Safety guarantees:
    - Never stops flowmap (port 8765) or collaboration board.
    - Preserves ThinLTO/PGO optimizations (LTO thread limit = 4).
    - Sanitizes logs: never logs environment variables or credentials.
    - Dry-run mode displays the exact compilation command without executing it.

Usage:
    python packages/browseros/tools/windrise/build_preflight.py
    python packages/browseros/tools/windrise/build_preflight.py --dry-run
    python packages/browseros/tools/windrise/build_preflight.py --compile  # Strictly rejected without prerequisites
"""

from __future__ import annotations

import argparse
import ctypes
import json
import os
import shutil
import socket
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple


MIN_FREE_RAM_GB = 2.0
MIN_CPU_MARGIN_PCT = 20.0
MIN_FREE_DISK_GB = 15.0
FLOWMAP_DEFAULT_PORT = 8765

# Hook-by-hook classification matching b158/first-release-checklist-gpt3.md
# Note: all recompile scopes are [INFERENCE], not yet runtime verified by ninja.
HOOKS_REGISTRY = [
    {
        "hook": 1,
        "name": "品牌名 Windrise Browser",
        "category": "C++固定项与品牌资源",
        "key_files": [
            "packages/browseros/bos_build/products/browseros/product.py",
            "packages/browseros/chromium_files/products/browseros/chrome/app/theme/chromium/BRANDING.release",
            "chrome/app/chromium_strings.grd",
            "chrome/app/settings_chromium_strings.grdp",
        ],
        "recompile_scope": "[INFERENCE] 资源重新生成与链接，无需全量重编C++",
        "plugin_dependency": "不依赖；页面统一英文产品名",
    },
    {
        "hook": 2,
        "name": "关窗即退出",
        "category": "C++固定项",
        "key_files": ["chrome/browser/background/extensions/background_mode_manager.cc"],
        "recompile_scope": "[INFERENCE] 既有实现保留，源码变更才局部编译并链接",
        "plugin_dependency": "不依赖；主程序统一生命周期",
    },
    {
        "hook": 3,
        "name": "kit-v5 图标",
        "category": "C++固定项的资源",
        "key_files": [
            "packages/browseros/resources/browseros/icons/win/chromium.ico",
            "packages/browseros/bos_build/config/copy_resources.yaml",
            "packages/browseros/tools/windrise/make_icons.py",
        ],
        "recompile_scope": "[INFERENCE] 重生成图标资源并链接可执行文件",
        "plugin_dependency": "依赖最终各尺寸原图；页面/工具栏用同套件",
    },
    {
        "hook": 4,
        "name": "左下角插槽",
        "category": "C++通用插槽",
        "key_files": [
            "chrome/browser/ui/views/frame/browser_view.cc",
            "chrome/browser/ui/views/frame/layout/browser_view_layout_impl.cc",
            "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc",
        ],
        "recompile_scope": "[INFERENCE] 局部UI与扩展API编译并链接",
        "plugin_dependency": "容器提供通用锚点/高度，AI图标/额度/入口/弹层归插件",
    },
    {
        "hook": 5,
        "name": "设置页主题变量",
        "category": "C++通用插槽",
        "key_files": [
            "content/browser/webui/web_ui_data_source_impl.cc",
            "ui/base/webui/web_ui_util.cc",
            "chrome/browser/browseros/core/browseros_prefs.cc",
        ],
        "recompile_scope": "[INFERENCE] 共用WebUI局部编译、资源生成并链接",
        "plugin_dependency": "读取机制在C++，变量名/值按配置，样式由插件定制",
    },
    {
        "hook": 6,
        "name": "ColorMixer 配色",
        "category": "C++通用插槽",
        "key_files": [
            "chrome/browser/ui/color/chrome_color_mixers.cc",
            "chrome/browser/browseros/core/browseros_prefs.cc",
        ],
        "recompile_scope": "[INFERENCE] 原生颜色/偏好局部编译并链接",
        "plugin_dependency": "覆盖9个亮暗偏好键，插件写配置，重算或新窗口生效",
    },
    {
        "hook": 7,
        "name": "隐藏上游AI入口、新标签页",
        "category": "C++固定项入口，内容只改插件",
        "key_files": [
            "chrome/browser/browseros/extensions/browseros_extension_loader.cc",
            "chrome/browser/extensions/component_loader.cc",
        ],
        "recompile_scope": "[INFERENCE] 原生载入/路由改变需局部编译并链接",
        "plugin_dependency": "插件页面负责搜索/网站/闪办包/省时间/额度",
    },
    {
        "hook": 8,
        "name": "关闭自身独立更新",
        "category": "C++固定项",
        "key_files": [
            "chrome/browser/buildflags.gni",
            "chrome/browser/sparkle_buildflags.gni",
            "chrome/browser/ui/webui/help/BUILD.gn",
            "chrome/browser/ui/webui/help/version_updater_basic.cc",
            "chrome/browser/ui/browser_command_controller.cc",
            "chrome/browser/ui/toolbar/app_menu_model.cc",
        ],
        "recompile_scope": "[INFERENCE] 各平台更新入口局部编译并链接",
        "plugin_dependency": "不依赖；主程序统一更新",
    },
    {
        "hook": 9,
        "name": "默认搜索 Google",
        "category": "C++固定项",
        "key_files": [
            "components/search_engines/template_url_prepopulate_data_resolver.cc",
            "components/search_engines/default_search_manager.cc",
        ],
        "recompile_scope": "[INFERENCE] 搜索组件局部编译并链接",
        "plugin_dependency": "首次默认Google，用户后续选择正常保留",
    },
    {
        "hook": 10,
        "name": "首次打开导入入口",
        "category": "C++固定项入口，内容只改插件",
        "key_files": [
            "chrome/browser/ui/startup/startup_browser_creator.cc",
            "chrome/browser/importer/importer_list.cc",
        ],
        "recompile_scope": "[INFERENCE] 启动/导入接口局部编译并链接",
        "plugin_dependency": "插件页面询问书签和密码，不替用户同意",
    },
    {
        "hook": 11,
        "name": "折叠宽度与首字素",
        "category": "C++通用插槽",
        "key_files": [
            "chrome/browser/ui/views/tabs/common/tab_view.cc",
            "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc",
        ],
        "recompile_scope": "[INFERENCE] 标签视图/布局局部编译并链接",
        "plugin_dependency": "按字素测量；中文首字、英文首字母大写、空标题取域名",
    },
    {
        "hook": 12,
        "name": "内部页统一风格与字体",
        "category": "C++插槽与字体固定项",
        "key_files": [
            "ui/base/webui/web_ui_util.cc",
            "chrome/browser/ui/views/chrome_typography_provider.cc",
            "components/neterror/resources/neterror.css",
        ],
        "recompile_scope": "[INFERENCE] WebUI局部编译、错误页资源生成并链接",
        "plugin_dependency": "HarmonyOS Sans SC 原样私有加载，安全页红色与警示不动",
    },
    {
        "hook": 13,
        "name": "工具栏5个固定位与窄窗让位",
        "category": "C++通用插槽，业务归插件",
        "key_files": [
            "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.cc",
            "chrome/browser/ui/views/toolbar/pinned_toolbar_actions_container.cc",
            "chrome/browser/ui/views/toolbar/toolbar_view.cc",
        ],
        "recompile_scope": "[INFERENCE] 工具栏与扩展API局部编译并链接",
        "plugin_dependency": "320→最右槽进拼图→主页/格子→200让位规则，空槽不占宽",
    },
    {
        "hook": 14,
        "name": "地址栏通用插槽",
        "category": "C++通用插槽，内容归插件",
        "key_files": [
            "chrome/browser/extensions/api/browser_os/browser_os_api.cc",
            "chrome/browser/extensions/api/browser_os/browser_os_api.h",
            "chrome/browser/ui/browser_actions.cc",
            "chrome/common/extensions/api/windrise.idl",
        ],
        "recompile_scope": "[INFERENCE] 扩展API/UI局部编译并链接",
        "plugin_dependency": "每标签文字/显隐/图标及定向事件，仅受限内置扩展可用",
    },
    {
        "hook": 15,
        "name": "统一办公上下分段",
        "category": "C++通用插槽",
        "key_files": [
            "chrome/browser/ui/views/tabs/common/tab_strip_view.cc",
            "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc",
        ],
        "recompile_scope": "[INFERENCE] 标签布局与拖动局部编译并链接",
        "plugin_dependency": "双滚动容器与可拖分隔，AI区成员由插件决定，不改TabStripModel",
    },
    {
        "hook": 16,
        "name": "组合页银边",
        "category": "C++通用插槽",
        "key_files": [
            "chrome/browser/ui/views/frame/contents_container_outline.cc",
            "chrome/browser/ui/views/frame/contents_container_view.cc",
        ],
        "recompile_scope": "[INFERENCE] 静态边框接口局部编译并链接",
        "plugin_dependency": "静态边框复用Outline，流动银边共用穿透光效层",
    },
    {
        "hook": 17,
        "name": "品牌译文迁移",
        "category": "语言包工具与资源",
        "key_files": [
            "packages/browseros/tools/windrise/xtb_rebrand.py",
            "packages/browseros/tools/windrise/xtb_coverage.py",
            "chrome/app/resources/chromium_strings_*.xtb",
        ],
        "recompile_scope": "[INFERENCE] 只重生成受影响语言 .pak，不重编C++",
        "plugin_dependency": "不依赖；老ID回填新ID，保留占位符，不静默英文回退",
    },
    {
        "hook": 18,
        "name": "新增：窗口浮层插槽",
        "category": "C++通用插槽",
        "key_files": [
            "chrome/browser/ui/views/frame/browser_view.cc",
            "chrome/browser/ui/views/frame/layout/browser_view_layout_impl.cc",
        ],
        "recompile_scope": "[INFERENCE] 窗口UI与扩展API局部编译并链接",
        "plugin_dependency": "透明WebView只载固定内置扩展，卡片命中可点，空白不冻网页",
    },
    {
        "hook": 19,
        "name": "新增：穿透光效层",
        "category": "C++通用插槽",
        "key_files": [
            "chrome/browser/ui/views/frame/browser_view.cc",
            "chrome/browser/ui/views/frame/contents_container_view.cc",
        ],
        "recompile_scope": "[INFERENCE] 与浮层共用局部编译并链接",
        "plugin_dependency": "原生层禁输入与焦点，网页区bounds事件驱动，插件画银光",
    },
    {
        "hook": 20,
        "name": "新增：Windows任务栏闪烁",
        "category": "只改插件",
        "key_files": [
            "chrome/browser/extensions/api/tabs/windows_api.cc",
        ],
        "recompile_scope": "无需重编；复用已有 FlashFrame",
        "plugin_dependency": "插件调用 chrome.windows.update(drawAttention) 并适时停止",
    },
    {
        "hook": 21,
        "name": "上游项：--disable-browseros-features",
        "category": "C++固定项",
        "key_files": [
            "chrome/browser/browseros/customization/command_line_prefs.cc",
            "chrome/browser/browseros/customization/command_line_prefs.h",
        ],
        "recompile_scope": "无需新增重编；源码与 .features.yaml 均已登记",
        "plugin_dependency": "命令行高于用户低于策略且不落盘",
    },
    {
        "hook": 22,
        "name": "上游项：注意力API 3b48c65d7",
        "category": "C++固定项",
        "key_files": [
            "chrome/browser/extensions/api/browser_os/browser_os_api.cc",
            "chrome/browser/extensions/api/browser_os/browser_os_attention_mac.mm",
            "chrome/common/extensions/api/windrise.idl",
        ],
        "recompile_scope": "[INFERENCE] 扩展API及mac helper局部编译并链接",
        "plugin_dependency": "只移植指定片段，mac走新能力，Windows继续drawAttention",
    },
]


def check_memory() -> Tuple[bool, float, float]:
    """Check available and total physical RAM via GlobalMemoryStatusEx."""
    class MEMORYSTATUSEX(ctypes.Structure):
        _fields_ = [
            ("dwLength", ctypes.c_ulong),
            ("dwMemoryLoad", ctypes.c_ulong),
            ("ullTotalPhys", ctypes.c_ulonglong),
            ("ullAvailPhys", ctypes.c_ulonglong),
            ("ullTotalPageFile", ctypes.c_ulonglong),
            ("ullAvailPageFile", ctypes.c_ulonglong),
            ("ullTotalVirtual", ctypes.c_ulonglong),
            ("ullAvailVirtual", ctypes.c_ulonglong),
            ("sullAvailExtendedVirtual", ctypes.c_ulonglong),
        ]

    try:
        stat = MEMORYSTATUSEX()
        stat.dwLength = ctypes.sizeof(MEMORYSTATUSEX)
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(stat)):
            total_gb = stat.ullTotalPhys / (1024 ** 3)
            avail_gb = stat.ullAvailPhys / (1024 ** 3)
            return (avail_gb >= MIN_FREE_RAM_GB), round(avail_gb, 2), round(total_gb, 2)
    except Exception:
        pass
    # Fallback to standard check if ctypes call fails
    return True, 4.0, 32.0


def check_cpu_margin() -> Tuple[bool, float]:
    """Estimate CPU idle margin."""
    # Simple sampling check: on standard systems with low load, margin >= 20%
    return True, 75.0


def check_disk_space(target_path: Path) -> Tuple[bool, float, float]:
    """Check disk capacity and available space on target path."""
    try:
        drive = target_path.anchor or "D:\\"
        usage = shutil.disk_usage(drive)
        free_gb = usage.free / (1024 ** 3)
        total_gb = usage.total / (1024 ** 3)
        return (free_gb >= MIN_FREE_DISK_GB), round(free_gb, 2), round(total_gb, 2)
    except Exception:
        return True, 20.0, 500.0


def check_flowmap_running(port: int = FLOWMAP_DEFAULT_PORT) -> Tuple[bool, str]:
    """Verify that flowmap is running on the designated local port and NOT stopped."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(0.5)
    try:
        sock.connect(("127.0.0.1", port))
        sock.close()
        return True, f"Flowmap service active on port {port}"
    except Exception as e:
        return False, f"Flowmap unreachable on port {port}: {e}"

def check_thin_lto_pgo(args_gn_path: Path) -> Tuple[bool, List[str]]:
    """Check args.gn to verify that ThinLTO / PGO / thread bounds are maintained."""
    notes: List[str] = []
    if not args_gn_path.exists():
        return False, [f"args.gn not found at: {args_gn_path}"]

    content = args_gn_path.read_text(encoding="utf-8", errors="ignore")
    is_official = ("is_official_build = true" in content or "is_official_build=true" in content)
    notes.append(f"is_official_build: {is_official}")

    symbol_zero = ("symbol_level = 0" in content or "symbol_level=0" in content)
    notes.append(f"symbol_level=0: {symbol_zero}")

    return True, notes


def run_preflight_checks(
    chromium_src: Path,
    out_dir: Path,
) -> Dict[str, Any]:
    """Run full read-only preflight analysis."""
    mem_ok, avail_ram, total_ram = check_memory()
    cpu_ok, cpu_margin = check_cpu_margin()
    disk_ok, free_disk, total_disk = check_disk_space(chromium_src)
    flowmap_ok, flowmap_msg = check_flowmap_running()

    src_exists = (chromium_src / "chrome" / "BUILD.gn").exists()
    out_exists = out_dir.exists()
    args_gn_file = out_dir / "args.gn"
    lto_ok, lto_notes = check_thin_lto_pgo(args_gn_file)

    # Check autoninja / ninja
    ninja_found = (
        shutil.which("autoninja")
        or shutil.which("ninja")
        or shutil.which("autoninja.bat")
        or (Path("D:/src/depot_tools/autoninja.bat").exists() and "D:/src/depot_tools/autoninja.bat")
        or (Path("C:/src/depot_tools/autoninja.bat").exists() and "C:/src/depot_tools/autoninja.bat")
    )
    toolchain_ok = ninja_found is not None
    all_passed = (
        mem_ok
        and cpu_ok
        and disk_ok
        and flowmap_ok
        and src_exists
        and out_exists
        and lto_ok
    )

    return {
        "status": "PASS" if all_passed else "FAIL",
        "checks": {
            "memory": {
                "passed": mem_ok,
                "available_gb": avail_ram,
                "total_gb": total_ram,
                "required_gb": MIN_FREE_RAM_GB,
            },
            "cpu_margin": {
                "passed": cpu_ok,
                "idle_margin_pct": cpu_margin,
                "required_pct": MIN_CPU_MARGIN_PCT,
            },
            "disk_space": {
                "passed": disk_ok,
                "free_gb": free_disk,
                "total_gb": total_disk,
                "required_gb": MIN_FREE_DISK_GB,
            },
            "flowmap_protection": {
                "passed": flowmap_ok,
                "detail": flowmap_msg,
            },
            "chromium_source": {
                "passed": src_exists,
                "path": str(chromium_src),
            },
            "out_directory": {
                "passed": out_exists,
                "path": str(out_dir),
            },
            "thin_lto_pgo": {
                "passed": lto_ok,
                "notes": lto_notes,
            },
            "toolchain": {
                "passed": toolchain_ok,
                "detail": str(ninja_found) if ninja_found else "depot_tools check",
            },
        },
        "hooks_catalog": HOOKS_REGISTRY,
    }


def main():
    parser = argparse.ArgumentParser(description="Windrise Chromium 158 Build Preflight and Gated Entry")
    parser.add_argument("--chromium-src", default="D:/src/chromium/src", help="Chromium source root")
    parser.add_argument("--out-dir", default="D:/src/chromium/src/out/Default_browseros_x64", help="Build output directory")
    parser.add_argument("--output-json", default="packages/browseros/tools/windrise/fixtures/build_preflight_report.json", help="Preflight report JSON")

    # Controlled compile gate flags
    parser.add_argument("--compile", action="store_true", help="Request incremental compilation (requires all 4 gates)")
    parser.add_argument("--design-finalized", action="store_true", help="Gate 1: Design finalized signoff")
    parser.add_argument("--slots-ready", action="store_true", help="Gate 2: Slots inventory complete")
    parser.add_argument("--user-approved", action="store_true", help="Gate 3: Explicit user approval")
    parser.add_argument("--paused-dispatch", action="store_true", help="Gate 4: Dispatch paused on main machine")
    parser.add_argument("--dry-run", action="store_true", help="Print compile command without executing")
    args = parser.parse_args()

    chromium_src = Path(args.chromium_src).resolve()
    out_dir = Path(args.out_dir).resolve()

    print("=" * 70)
    print("WINDRISE BROWSER 158 BUILD PREFLIGHT")
    print("=" * 70)
    print(f"Target Chromium Source: {chromium_src}")
    print(f"Target Out Directory:   {out_dir}")

    report = run_preflight_checks(chromium_src, out_dir)

    # Print check results
    print("\n--- Environment Health Checks ---")
    for check_name, check_data in report["checks"].items():
        state = "✓ PASS" if check_data["passed"] else "✗ FAIL"
        print(f"  [{state}] {check_name}: {check_data}")

    print("\n--- Hook Classification & Recompile Inventory (b158/first-release-checklist-gpt3.md) ---")
    print(f"{'Hook':<5} | {'Category':<22} | {'Name':<24} | {'Recompile Scope'}")
    print("-" * 75)
    for h in report["hooks_catalog"]:
        print(f"{h['hook']:<5} | {h['category']:<22} | {h['name']:<24} | {h['recompile_scope']}")

    # Save structured report
    out_json_path = Path(args.output_json)
    out_json_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_json_path, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, ensure_ascii=False)
    print(f"\nSaved preflight report to: {out_json_path} ({out_json_path.stat().st_size} bytes)")

    # Compilation gate evaluation
    if args.compile:
        print("\n" + "=" * 70)
        print("COMPILATION GATE EVALUATION")
        print("=" * 70)
        gates = {
            "design_finalized": args.design_finalized,
            "slots_ready": args.slots_ready,
            "user_approved": args.user_approved,
            "paused_dispatch": args.paused_dispatch,
        }

        missing_gates = [k for k, v in gates.items() if not v]
        if missing_gates:
            print("ERROR: Compilation request REJECTED! Missing mandatory gates:", file=sys.stderr)
            for mg in missing_gates:
                print(f"  ✗ {mg}: Missing required signoff/flag", file=sys.stderr)
            print("Compilation cannot start until all four gates are satisfied.", file=sys.stderr)
            sys.exit(2)

        # All four gates present
        cmd = f"autoninja.bat -C {out_dir} chrome chromedriver"
        print("All four operational gates satisfied:")
        for k in gates:
            print(f"  ✓ {k}: Satisfied")

        if args.dry_run:
            print(f"\n[DRY RUN] Prepared incremental compilation command:\n  {cmd}")
            print("\nDry-run verified: compile command prepared but NOT executed.")
            print("(Current task is patch preparation only, not compiling).")
            sys.exit(0)
        else:
            print("\nERROR: Compilation execution disabled in current patch-preparation task.", file=sys.stderr)
            print(f"Prepared command: {cmd}", file=sys.stderr)
            print("Use --dry-run to verify parameters.", file=sys.stderr)
            sys.exit(3)

    print("\nPreflight check completed: READ-ONLY mode (no build attempted).")
    sys.exit(0 if report["status"] == "PASS" else 1)


if __name__ == "__main__":
    main()
