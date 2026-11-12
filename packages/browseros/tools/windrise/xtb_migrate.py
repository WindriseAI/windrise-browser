#!/usr/bin/env python3
"""Hook 17 XTB 译文品牌迁移落地（写回 xtb 源）。

在 `xtb_coverage.py` 的分析结果上，把「可恢复」的消息（当前 ID 缺失、但上游旧
ID 在语言包里）迁移到当前 ID，并写回 `chrome/app/resources/*.xtb`：保留原有
条目，新增条目按 id 升序插入。

替换词不靠猜：对每条消息，取上游英文原文与当前英文原文，穷举品牌词的组合，只有
能把上游原文逐字变成当前原文的那一组才用（例如 "Chrome" -> "Windrise Browser"）。
对不上的一律不迁移，只进报告。`xtb_rebrand.rebrand_translations` 负责实际的文本
替换（含占位符尾部，并保护 ChromiumOS / ChromeOS）。

用法:
    python packages/browseros/tools/windrise/xtb_migrate.py \
        --chromium-src D:/src/chromium/src --lang zh-CN --dry-run
    ... --lang zh-CN            # 实际写回
"""

from __future__ import annotations

import argparse
import itertools
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

try:
    from .xtb_coverage import (
        RESOURCE_CONFIGS,
        STANDARD_DEFINES,
        analyze_coverage,
        load_grit,
        parse_grd_messages,
    )
    from .xtb_rebrand import rebrand_translations
except ImportError:  # 直接按脚本跑
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from xtb_coverage import (  # type: ignore
        RESOURCE_CONFIGS,
        STANDARD_DEFINES,
        analyze_coverage,
        load_grit,
        parse_grd_messages,
    )
    from xtb_rebrand import rebrand_translations  # type: ignore

# 品牌词，按长度降序（长词先替换，避免 "Google Chrome" 被 "Chrome" 吃掉一半）。
BRAND_TOKENS = (
    "Google Chrome",
    "Chromium OS",
    "Chrome OS",
    "Chromium",
    "Chrome",
    "Google",
)

REPLACEMENT = "Windrise Browser"


def _apply_pairs(text: str, pairs: Sequence[Tuple[str, str]]) -> str:
    for old, new in pairs:
        text = text.replace(old, new)
    return text


def derive_replacements(old_text: str, new_text: str) -> Optional[List[Tuple[str, str]]]:
    """找出把上游英文原文变成当前英文原文的品牌替换组合；找不到返回 None。"""
    if old_text == new_text:
        return []
    present = [token for token in BRAND_TOKENS if token in old_text]
    for size in range(1, len(present) + 1):
        for combo in itertools.combinations(present, size):
            pairs = [(token, REPLACEMENT) for token in combo]
            if _apply_pairs(old_text, pairs) == new_text:
                return pairs
    return None


def _load_upstream_texts(
    chromium_src: Path, upstream_commit: str, res_key: str
) -> Dict[str, str]:
    """上游（基线提交）里每个消息名对应的英文原文。"""
    cfg = RESOURCE_CONFIGS[res_key]
    grd_rel = cfg["grd"]
    subparts = cfg.get("subparts", [])
    grd_reader, M, _ = load_grit(chromium_src)

    if res_key != "chromium_strings":
        grd_path = chromium_src / grd_rel
        nodes = parse_grd_messages(
            grd_reader, M, grd_path, grd_path.parent, STANDARD_DEFINES
        )
        return _messages_to_text(nodes, M)

    temp_dir = tempfile.mkdtemp(prefix="grit_upstream_")
    try:
        for rel in [grd_rel] + subparts:
            out_p = Path(temp_dir) / rel
            out_p.parent.mkdir(parents=True, exist_ok=True)
            res = subprocess.run(
                ["git", "cat-file", "-p", f"{upstream_commit}:{rel}"],
                cwd=chromium_src,
                capture_output=True,
            )
            if res.returncode == 0:
                out_p.write_bytes(res.stdout)
        target_grd = Path(temp_dir) / grd_rel
        if not target_grd.exists():
            return {}
        nodes = parse_grd_messages(
            grd_reader, M, target_grd, target_grd.parent, STANDARD_DEFINES
        )
        return _messages_to_text(nodes, M)
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


def _messages_to_text(nodes, M) -> Dict[str, str]:
    texts: Dict[str, str] = {}
    for node in nodes:
        if isinstance(node, M.MessageNode):
            message = node.GetCliques()[0].GetMessage()
            texts[node.attrs["name"]] = message.GetPresentableContent()
    return texts


def _insert_sorted(xml: str, entries: Dict[str, str]) -> str:
    """把 id -> 译文 的条目按 id 升序插进 xtb（保留原有条目与 CRLF）。"""
    if not entries:
        return xml
    lines = xml.split("\n")
    has_crlf = bool(lines[0].endswith("\r")) if lines else False
    # 闭合标签之前是最后一个合法插入点：id 比现有全部条目都大时必须落在这里，
    # 不能落到 </translationbundle> 之后的文档外。
    close_index = next(
        (i for i, line in enumerate(lines) if line.strip().startswith("</translationbundle>")),
        len(lines),
    )
    for new_id in sorted(entries, key=int):
        entry = '<translation id="%s">%s</translation>' % (new_id, entries[new_id])
        index = close_index
        for i in range(close_index):
            match = re.match(r'<translation id="(\d+)"', lines[i])
            if match and int(match.group(1)) > int(new_id):
                index = i
                break
        lines.insert(index, entry + ("\r" if has_crlf else ""))
        close_index += 1
    return "\n".join(lines)


def migrate_language(
    chromium_src: Path,
    upstream_commit: str,
    lang: str,
    dry_run: bool,
) -> Dict:
    report = analyze_coverage(
        chromium_src, upstream_commit, list(RESOURCE_CONFIGS.keys()), [lang]
    )

    # 资源 -> 消息名 -> 上游英文原文
    upstream_texts: Dict[str, Dict[str, str]] = {}
    for res_key in RESOURCE_CONFIGS:
        upstream_texts[res_key] = _load_upstream_texts(
            chromium_src, upstream_commit, res_key
        )

    # resource_path -> 明细
    per_file: Dict[str, List[Dict]] = {}
    for detail in report["all_details"]:
        if detail["processing_result"] == "recoverable":
            per_file.setdefault(detail["resource_path"], []).append(detail)

    summary: Dict[str, object] = {
        "language": lang,
        "by_family": Counter(),
        "migrated": {},
        "skipped": [],
    }

    for rel_path, details in per_file.items():
        res_key = next(
            (
                key
                for key, cfg in RESOURCE_CONFIGS.items()
                if rel_path.startswith(cfg["xtb_dir"] + "/")
            ),
            None,
        )
        old_texts = upstream_texts.get(res_key, {}) if res_key else {}

        xtb_path = chromium_src / rel_path
        original = xtb_path.read_text(encoding="utf-8", errors="ignore")

        candidates: Dict[str, str] = {}
        for detail in details:
            old_text = old_texts.get(detail["message_name"])
            if old_text is None:
                summary["skipped"].append(
                    {"message": detail["message_name"], "reason": "上游没有这个字符串"}
                )
                continue
            pairs = derive_replacements(old_text, detail["english_text"])
            if pairs is None:
                summary["skipped"].append(
                    {
                        "message": detail["message_name"],
                        "reason": "上游原文到当前原文不是纯品牌替换",
                    }
                )
                continue
            if not pairs:
                summary["skipped"].append(
                    {"message": detail["message_name"], "reason": "英文原文没变"}
                )
                continue

            migrated = rebrand_translations(
                original,
                [
                    {
                        "id": detail["current_id"],
                        "legacy_ids": [detail["translation_source"]],
                    }
                ],
                dict(pairs),
            )
            if detail["current_id"] in migrated["missing"]:
                summary["skipped"].append(
                    {"message": detail["message_name"], "reason": "旧 ID 没在 xtb 里"}
                )
                continue
            match = re.search(
                r'<translation id="%s">(.*?)</translation>'
                % re.escape(detail["current_id"]),
                migrated["xml"],
                re.S,
            )
            if not match:
                summary["skipped"].append(
                    {"message": detail["message_name"], "reason": "迁移结果里取不到该 id"}
                )
                continue
            candidates[detail["current_id"]] = match.group(1)
            summary["by_family"][",".join(old for old, _ in pairs)] += 1  # type: ignore[index]

        summary["migrated"][rel_path] = len(candidates)  # type: ignore[index]
        if dry_run or not candidates:
            continue
        xtb_path.write_text(_insert_sorted(original, candidates), encoding="utf-8")

    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description="写回风桥品牌译文（xtb 源）")
    parser.add_argument("--chromium-src", default="D:/src/chromium/src")
    parser.add_argument(
        "--upstream-commit",
        default="6449d9f9a6a906edff724b7b220199b3fde4615c",
    )
    parser.add_argument("--lang", action="append", default=None)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    langs: Sequence[str] = args.lang or ["zh-CN", "zh-TW", "zh-HK"]
    src = Path(args.chromium_src)
    for lang in langs:
        summary = migrate_language(src, args.upstream_commit, lang, args.dry_run)
        print(f"\n== {lang} ==")
        print("  替换词统计:", dict(summary["by_family"]))
        print("  每个文件新增:", summary["migrated"])
        print("  跳过:", len(summary["skipped"]))
        for item in summary["skipped"][:10]:
            print("   -", item["message"], item["reason"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
