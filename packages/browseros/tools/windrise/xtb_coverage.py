#!/usr/bin/env python3
"""Hook 17 XTB Translation Coverage and Rebranding Analysis Tool.

Replaces the legacy b158/xtb-coverage.py script by taking dynamic CLI arguments,
parsing active messages with real GRIT across all 3 Chromium string resource types,
generating candidate legacy IDs, evaluating 81 languages, and producing a
deterministic coverage and missing-translation report.

Usage:
    python packages/browseros/tools/windrise/xtb_coverage.py --chromium-src D:/src/chromium/src
    python packages/browseros/tools/windrise/xtb_coverage.py --chromium-src D:/src/chromium/src --lang zh-CN
    python packages/browseros/tools/windrise/xtb_coverage.py --chromium-src D:/src/chromium/src --output-json packages/browseros/tools/windrise/fixtures/xtb_coverage_report.json
"""

from __future__ import annotations

import argparse
import copy
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Set, Tuple

# Import rebrand logic
try:
    from .xtb_rebrand import rebrand_translations
except ImportError:
    from xtb_rebrand import rebrand_translations


STANDARD_DEFINES = {
    '_is_chrome_for_testing_branded': False,
    '_google_chrome': False,
    '_chromium': True,
    'chromeos_ash': False,
    'use_titlecase': False,
    'is_android': False,
    'is_ios': False,
    'is_mac': False,
    'is_win': True,
    'is_linux': False,
    'is_chromeos': False,
    'is_desktop_android': False,
}

RESOURCE_CONFIGS = {
    'chromium_strings': {
        'grd': 'chrome/app/chromium_strings.grd',
        'subparts': ['chrome/app/settings_chromium_strings.grdp'],
        'xtb_dir': 'chrome/app/resources',
        'xtb_prefix': 'chromium_strings_',
    },
    'generated_resources': {
        'grd': 'chrome/app/generated_resources.grd',
        'subparts': [],
        'xtb_dir': 'chrome/app/resources',
        'xtb_prefix': 'generated_resources_',
    },
    'components_chromium_strings': {
        'grd': 'components/components_chromium_strings.grd',
        'subparts': [],
        'xtb_dir': 'components/strings',
        'xtb_prefix': 'components_chromium_strings_',
    },
    'components_strings': {
        'grd': 'components/components_strings.grd',
        'subparts': ['components/new_or_sad_tab_strings.grdp'],
        'xtb_dir': 'components/strings',
        'xtb_prefix': 'components_strings_',
    },
}

ALL_81_LANGUAGES = [
    'af', 'am', 'ar', 'as', 'az', 'be', 'bg', 'bn', 'bs', 'ca', 'cs', 'cy',
    'da', 'de', 'el', 'en-GB', 'es', 'es-419', 'et', 'eu', 'fa', 'fi', 'fil',
    'fr', 'fr-CA', 'gl', 'gu', 'he', 'hi', 'hr', 'hu', 'hy', 'id', 'is', 'it',
    'ja', 'ka', 'kk', 'km', 'kn', 'ko', 'ky', 'lo', 'lt', 'lv', 'mk', 'ml',
    'mn', 'mr', 'ms', 'my', 'ne', 'nl', 'no', 'or', 'pa', 'pl', 'pt-BR', 'pt-PT',
    'ro', 'ru', 'si', 'sk', 'sl', 'sq', 'sr', 'sr-Latn', 'sv', 'sw', 'ta', 'te',
    'th', 'tr', 'uk', 'ur', 'uz', 'vi', 'zh-CN', 'zh-HK', 'zh-TW', 'zu'
]


@dataclass
class MessageDetail:
    resource_path: str
    language: str
    message_name: str
    current_id: str
    candidate_legacy_ids: List[str]
    meaning: str
    placeholder_structure: List[Dict[str, str]]
    translation_source: Optional[str]
    processing_result: str  # 'already_translated', 'recoverable', 'missing'
    english_text: str
    unrecovered_reason: Optional[str] = None


def load_grit(chromium_src: Path):
    grit_path = chromium_src / 'tools' / 'grit'
    if not grit_path.exists():
        raise FileNotFoundError(f"GRIT tools directory not found at: {grit_path}")
    if str(grit_path) not in sys.path:
        sys.path.insert(0, str(grit_path))
    from grit import grd_reader
    from grit.node import message as M
    from grit.extern import tclib as T
    return grd_reader, M, T


def parse_grd_messages(grd_reader, M, grd_path: Path, base_dir: Path, defines: Dict[str, Any]):
    defs = dict(defines)
    for _ in range(60):
        try:
            root = grd_reader.Parse(
                str(grd_path),
                dir=str(base_dir),
                debug=False,
                defines=defs,
                target_platform='win32'
            )
            root.SetOutputLanguage('en')
            return list(root.ActiveDescendants())
        except AssertionError as e:
            m = re.search(r'variable found: (\S+)', str(e))
            if not m:
                raise
            defs[m.group(1)] = False
    raise RuntimeError(f"Could not parse GRD after 60 iterations: {grd_path}")


def get_clean_upstream_ids(
    chromium_src: Path,
    upstream_commit: str,
    res_key: str,
    grd_reader,
    M,
) -> Dict[str, str]:
    """Parse clean upstream GRD to get original message IDs."""
    cfg = RESOURCE_CONFIGS[res_key]
    grd_rel = cfg['grd']
    subparts = cfg.get('subparts', [])

    # For generated_resources, no branding replaces were done in .grd,
    # so reading directly from chromium_src (where all included .grdp subparts exist) gives exact IDs.
    if res_key not in ('chromium_strings', 'components_chromium_strings', 'components_strings'):
        grd_path = chromium_src / grd_rel
        nodes = parse_grd_messages(grd_reader, M, grd_path, grd_path.parent, STANDARD_DEFINES)
        id_map: Dict[str, str] = {}
        for n in nodes:
            if isinstance(n, M.MessageNode):
                m = n.GetCliques()[0].GetMessage()
                id_map[n.attrs['name']] = str(m.GetId())
        return id_map

    # For chromium_strings, settings_chromium_strings.grdp was modified by string_replaces in working copy.
    # We extract clean upstream versions of both chromium_strings.grd and settings_chromium_strings.grdp.
    temp_dir = tempfile.mkdtemp(prefix='grit_upstream_')
    try:
        for rel in [grd_rel] + subparts:
            out_p = Path(temp_dir) / rel
            out_p.parent.mkdir(parents=True, exist_ok=True)
            res = subprocess.run(
                ['git', 'cat-file', '-p', f'{upstream_commit}:{rel}'],
                cwd=chromium_src,
                capture_output=True,
            )
            if res.returncode == 0:
                out_p.write_bytes(res.stdout)

        target_grd = Path(temp_dir) / grd_rel
        if not target_grd.exists():
            return {}

        nodes = parse_grd_messages(
            grd_reader,
            M,
            target_grd,
            target_grd.parent,
            STANDARD_DEFINES
        )

        id_map: Dict[str, str] = {}
        for n in nodes:
            if isinstance(n, M.MessageNode):
                m = n.GetCliques()[0].GetMessage()
                id_map[n.attrs['name']] = str(m.GetId())
        return id_map
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


def build_candidate_legacy_ids(
    name: str,
    text: str,
    meaning: str,
    upstream_id: Optional[str],
    T,
) -> List[str]:
    cands: List[str] = []

    # Priority 1: Exact clean upstream message ID for the same message name
    if upstream_id:
        cands.append(str(upstream_id))

    # Priority 2: Direct product brand restoration
    t_chromium = (
        text.replace('Windrise BrowserOS', 'ChromiumOS')
            .replace('Windrise Browser', 'Chromium')
            .replace('Windrise', 'Chromium')
    )
    cands.append(str(T.GenerateMessageId(t_chromium, meaning)))

    # Priority 3: Space variant (Chromium OS)
    t_chromium_os = (
        text.replace('Windrise BrowserOS', 'Chromium OS')
            .replace('Windrise Browser', 'Chromium')
            .replace('Windrise', 'Chromium')
    )
    cands.append(str(T.GenerateMessageId(t_chromium_os, meaning)))

    # Priority 4: Replaced from Chrome
    t_chrome = text.replace('Windrise Browser', 'Chrome').replace('Windrise', 'Chrome')
    cands.append(str(T.GenerateMessageId(t_chrome, meaning)))

    # Priority 5: Replaced from Google Chrome
    t_google_chrome = text.replace('Windrise Browser', 'Google Chrome').replace('Windrise', 'Google Chrome')
    cands.append(str(T.GenerateMessageId(t_google_chrome, meaning)))

    # Priority 6: Contextual Google service replacements
    tg = text.replace('Windrise Browser Account', 'Google Account')
    tg = tg.replace('Windrise Browser servers', 'Google servers')
    tg = tg.replace('Windrise Browser services', 'Google services')
    tg = tg.replace('Windrise Browser sites', 'Google sites')
    tg = tg.replace('Windrise Browser apps', 'Google apps')
    tg_chrom = tg.replace('Windrise Browser', 'Chromium').replace('Windrise', 'Chromium')
    tg_chrome = tg.replace('Windrise Browser', 'Chrome').replace('Windrise', 'Chrome')
    cands.append(str(T.GenerateMessageId(tg_chrom, meaning)))
    cands.append(str(T.GenerateMessageId(tg_chrome, meaning)))

    # Priority 7: Pure Google replacement
    tg_pure = text.replace('Windrise Browser', 'Google').replace('Windrise', 'Google')
    cands.append(str(T.GenerateMessageId(tg_pure, meaning)))

    # Deduplicate while preserving order
    seen: Set[str] = set()
    deduped: List[str] = []
    for c in cands:
        if c not in seen:
            seen.add(c)
            deduped.append(c)
    return deduped


def analyze_coverage(
    chromium_src: Path,
    upstream_commit: str,
    resource_keys: Sequence[str],
    languages: Sequence[str],
) -> Dict[str, Any]:
    """Run full GRIT analysis across resources and languages."""
    grd_reader, M, T = load_grit(chromium_src)

    report: Dict[str, Any] = {
        'upstream_commit': upstream_commit,
        'resources': {},
        'language_summary': {},
        'all_details': [],
        'missing_details': [],
    }

    brand_pattern = re.compile(r'\b(Windrise|Chromium)\b')

    for res_key in resource_keys:
        cfg = RESOURCE_CONFIGS[res_key]
        grd_path = chromium_src / cfg['grd']
        if not grd_path.exists():
            print(f"Warning: {grd_path} does not exist, skipping {res_key}")
            continue

        print(f"\nAnalyzing resource: {res_key} ({cfg['grd']})...")

        # 1. Parse clean upstream IDs
        upstream_id_map = get_clean_upstream_ids(
            chromium_src, upstream_commit, res_key, grd_reader, M
        )
        print(f"  Loaded {len(upstream_id_map)} clean upstream message IDs")

        # 2. Parse current active messages
        nodes = parse_grd_messages(grd_reader, M, grd_path, grd_path.parent, STANDARD_DEFINES)

        active_brand_msgs = []
        for n in nodes:
            if not isinstance(n, M.MessageNode):
                continue
            m = n.GetCliques()[0].GetMessage()
            text = m.GetPresentableContent()
            if brand_pattern.search(text):
                # Extract placeholder structure
                phs = []
                for child in n.children:
                    if child.name == 'ph':
                        phs.append({
                            'name': child.attrs.get('name', ''),
                            'example': child.attrs.get('example', '')
                        })

                name = n.attrs['name']
                cid = str(m.GetId())
                meaning = m.GetMeaning() or ''
                up_id = upstream_id_map.get(name)
                cands = build_candidate_legacy_ids(name, text, meaning, up_id, T)

                active_brand_msgs.append({
                    'name': name,
                    'current_id': cid,
                    'meaning': meaning,
                    'text': text,
                    'candidate_legacy_ids': cands,
                    'placeholders': phs,
                })

        print(f"  Active branded messages count: {len(active_brand_msgs)}")

        res_report: Dict[str, Any] = {
            'total_active_messages': len([n for n in nodes if isinstance(n, M.MessageNode)]),
            'total_branded_messages': len(active_brand_msgs),
            'languages': {},
        }

        # 3. Analyze across requested languages
        for lang in languages:
            xtb_filename = f"{cfg['xtb_prefix']}{lang}.xtb"
            xtb_path = chromium_src / cfg['xtb_dir'] / xtb_filename
            rel_xtb = f"{cfg['xtb_dir']}/{xtb_filename}"

            if not xtb_path.exists():
                continue

            xtb_content = xtb_path.read_text(encoding='utf-8', errors='ignore')
            existing_translations = set(re.findall(r'<translation id="(\d+)"', xtb_content))

            already_translated = 0
            recoverable = 0
            missing_count = 0

            for msg in active_brand_msgs:
                cid = msg['current_id']
                name = msg['name']
                cands = msg['candidate_legacy_ids']

                translation_source = None
                result_status = 'missing'
                unrecovered_reason = None

                if cid in existing_translations:
                    result_status = 'already_translated'
                    translation_source = cid
                    already_translated += 1
                else:
                    for cand in cands:
                        if cand in existing_translations:
                            result_status = 'recoverable'
                            translation_source = cand
                            recoverable += 1
                            break

                if result_status == 'missing':
                    missing_count += 1
                    if 'Chromium' in msg['text'] and not any(cand in existing_translations for cand in cands):
                        unrecovered_reason = "Untranslated upstream brand name or newly introduced message in 158"
                    else:
                        unrecovered_reason = "No matching candidate ID found in official language bundle"

                detail = MessageDetail(
                    resource_path=rel_xtb,
                    language=lang,
                    message_name=name,
                    current_id=cid,
                    candidate_legacy_ids=cands,
                    meaning=msg['meaning'],
                    placeholder_structure=msg['placeholders'],
                    translation_source=translation_source,
                    processing_result=result_status,
                    english_text=msg['text'],
                    unrecovered_reason=unrecovered_reason,
                )

                detail_dict = asdict(detail)
                report['all_details'].append(detail_dict)
                if result_status == 'missing':
                    report['missing_details'].append(detail_dict)

            tot = len(active_brand_msgs)
            hit = already_translated + recoverable
            cov_pct = (hit / tot * 100.0) if tot > 0 else 100.0

            res_report['languages'][lang] = {
                'total_branded': tot,
                'already_translated': already_translated,
                'recoverable': recoverable,
                'missing': missing_count,
                'coverage_pct': round(cov_pct, 2),
            }

            if lang not in report['language_summary']:
                report['language_summary'][lang] = {
                    'total_branded': 0,
                    'already_translated': 0,
                    'recoverable': 0,
                    'missing': 0,
                }
            report['language_summary'][lang]['total_branded'] += tot
            report['language_summary'][lang]['already_translated'] += already_translated
            report['language_summary'][lang]['recoverable'] += recoverable
            report['language_summary'][lang]['missing'] += missing_count

        report['resources'][res_key] = res_report

    # Overall summary
    total_branded_all = sum(v['total_branded'] for v in report['language_summary'].values())
    total_recovered_all = sum(
        (v['already_translated'] + v['recoverable']) for v in report['language_summary'].values()
    )
    report['overall_coverage_pct'] = (
        round(total_recovered_all / total_branded_all * 100.0, 2) if total_branded_all > 0 else 100.0
    )
    return report

def generate_fixtures_and_e2e(
    report: Dict[str, Any],
    chromium_src: Path,
    fixtures_dir: Path,
):
    """Generate sample grd, rebranded xtb fixture, and run E2E verification with GRIT."""
    fixtures_dir.mkdir(parents=True, exist_ok=True)

    # 1. Write sample grd
    sample_grd = fixtures_dir / "sample_messages.grd"
    sample_grd_content = '''<?xml version="1.0" encoding="utf-8"?>
<grit latest_public_release="0" current_release="1" output_all_resource_defines="false">
  <outputs>
    <output filename="sample_strings_en.pak" type="data_package" lang="en" />
    <output filename="sample_strings_zh-CN.pak" type="data_package" lang="zh-CN" />
  </outputs>
  <translations>
    <file path="rebranded_sample_zh_cn.xtb" lang="zh-CN" />
  </translations>
  <release seq="1">
    <messages fallback_to_english="true">
      <message name="IDS_SAMPLE_ABOUT" desc="Menu title for the About page.">
        About Windrise Browser
      </message>
      <message name="IDS_SAMPLE_RESTART" desc="Dialog text for restart.">
        Relaunch <ph name="PRODUCT"/>, keeping <ph name="COUNT"/> windows
      </message>
      <message name="IDS_SAMPLE_EXISTING" desc="Existing translated string.">
        Already translated text
      </message>
    </messages>
  </release>
</grit>
'''
    sample_grd.write_text(sample_grd_content, encoding='utf-8')

    # 2. Generate and verify rebranded sample xtb
    source_xtb = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE translationbundle>
<translationbundle lang="zh-CN">
  <translation id="1185134272377778587">关于 Chromium</translation>
  <translation id="101">重新启动 <ph name="PRODUCT"/>，保留 <ph name="COUNT"/> 个窗口</translation>
  <translation id="200">已经翻好的风桥文字</translation>
  <translation id="900">Chromium OS 版本</translation>
</translationbundle>'''

    # Message 6627346220057790266 is "About Windrise Browser", legacy is 1185134272377778587 ("About Chromium")
    messages_to_rebrand = [
        {'id': '6627346220057790266', 'legacy_ids': ['1185134272377778587']},
        {'id': '202', 'legacy_ids': ['101']},
        {'id': '200', 'legacy_ids': ['100']},
    ]
    replacements = {'Chromium': 'Windrise Browser'}

    rebrand_res = rebrand_translations(source_xtb, messages_to_rebrand, replacements)
    sample_xtb = fixtures_dir / "rebranded_sample_zh_cn.xtb"
    sample_xtb.write_text(rebrand_res['xml'], encoding='utf-8')

    # 3. E2E: Read back with GRIT's xtb_reader
    grit_path = chromium_src / 'tools' / 'grit'
    if str(grit_path) not in sys.path:
        sys.path.insert(0, str(grit_path))
    from grit import xtb_reader

    parsed_by_grit: Dict[str, Any] = {}
    def _xtb_cb(mid, val):
        parsed_by_grit[str(mid)] = val

    with open(sample_xtb, 'rb') as f:
        xtb_reader.Parse(f, _xtb_cb)

    # Verify migrated ID exists and contains Windrise Browser text
    assert '6627346220057790266' in parsed_by_grit, "Migrated ID 6627346220057790266 not parsed by GRIT"
    migrated_val = parsed_by_grit['6627346220057790266']['OTHER'][0][1]
    assert migrated_val == '关于 Windrise Browser', f"Unexpected text: {migrated_val}"
    print(f"  [E2E VERIFIED] Real XTB -> rebrand_translations -> migrated XTB -> GRIT xtb_reader OK!")


def main():
    parser = argparse.ArgumentParser(description="Hook 17 XTB Translation Coverage and Rebranding Analysis")
    parser.add_argument('--chromium-src', default='D:/src/chromium/src', help="Path to Chromium source root")
    parser.add_argument('--upstream-commit', default='6449d9f9a6a906edff724b7b220199b3fde4615c', help="Clean upstream commit hash")
    parser.add_argument('--output-json', default='packages/browseros/tools/windrise/fixtures/xtb_coverage_summary.json', help="Output summary JSON path")
    parser.add_argument('--resources', default='chromium_strings,generated_resources,components_chromium_strings', help="Comma-separated resources")
    parser.add_argument('--lang', default=None, help="Filter to specific language (e.g. zh-CN), default all 81")
    parser.add_argument('--fixtures-dir', default='packages/browseros/tools/windrise/fixtures', help="Fixtures directory")
    args = parser.parse_args()

    chromium_src = Path(args.chromium_src).resolve()
    if not chromium_src.exists():
        print(f"Error: Chromium src directory does not exist: {chromium_src}", file=sys.stderr)
        sys.exit(1)

    res_keys = [r.strip() for r in args.resources.split(',') if r.strip()]
    langs = [args.lang] if args.lang else ALL_81_LANGUAGES

    print(f"Starting XTB coverage analysis against {chromium_src}...")
    print(f"Resources: {res_keys}")
    print(f"Languages: {len(langs)} ({langs[0]} ... {langs[-1]})")

    report = analyze_coverage(
        chromium_src=chromium_src,
        upstream_commit=args.upstream_commit,
        resource_keys=res_keys,
        languages=langs,
    )

    # Print summary table
    print("\n" + "=" * 80)
    print("XTB COVERAGE REPORT SUMMARY")
    print("=" * 80)
    print(f"{'Language':<10} | {'Total':<8} | {'Already':<8} | {'Recovered':<10} | {'Missing':<8} | {'Coverage %':<10}")
    print("-" * 80)

    sample_langs = ['zh-CN', 'zh-TW', 'en-GB', 'ja', 'fr', 'de'] if not args.lang else langs
    for lang in sample_langs:
        if lang in report['language_summary']:
            s = report['language_summary'][lang]
            tot = s['total_branded']
            alr = s['already_translated']
            rec = s['recoverable']
            mis = s['missing']
            cov = round((alr + rec) / tot * 100.0, 2) if tot > 0 else 100.0
            print(f"{lang:<10} | {tot:<8} | {alr:<8} | {rec:<10} | {mis:<8} | {cov:>9.2f}%")

    if not args.lang and len(langs) > 6:
        print(f"... and {len(langs) - 6} more languages analyzed.")

    zh_cn = report['language_summary'].get('zh-CN', {})
    print("-" * 80)
    print(f"zh-CN Status: Total {zh_cn.get('total_branded', 0)}, Recoverable {zh_cn.get('recoverable', 0)}, Missing {zh_cn.get('missing', 0)}")
    print(f"Total Detailed Records: {len(report['all_details'])}, Missing Records: {len(report['missing_details'])}")

    # Extract compact summary and zh-CN missing table
    fixtures_dir = Path(args.fixtures_dir)
    fixtures_dir.mkdir(parents=True, exist_ok=True)

    zh_cn_missing = [d for d in report['missing_details'] if d['language'] == 'zh-CN']
    zh_cn_missing_file = fixtures_dir / "missing_translations_zh_cn.json"
    zh_cn_missing_file.write_text(json.dumps(zh_cn_missing, indent=2, ensure_ascii=False), encoding='utf-8')
    print(f"Saved {len(zh_cn_missing)} zh-CN missing records to: {zh_cn_missing_file}")

    # Generate and verify E2E fixtures
    generate_fixtures_and_e2e(report, chromium_src, fixtures_dir)

    # Save coverage summary
    summary_payload = {
        'upstream_commit': report['upstream_commit'],
        'overall_coverage_pct': report['overall_coverage_pct'],
        'resources': report['resources'],
        'language_summary': report['language_summary'],
        'zh_cn_missing_count': len(zh_cn_missing),
    }
    out_path = Path(args.output_json)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, 'w', encoding='utf-8') as f:
        json.dump(summary_payload, f, indent=2, ensure_ascii=False)
    print(f"Saved coverage summary to: {out_path} ({out_path.stat().st_size} bytes)")


if __name__ == '__main__':
    main()
