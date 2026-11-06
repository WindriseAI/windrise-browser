#!/usr/bin/env python3
"""Hook 17 Translation rebrand helper for Chromium/Windrise .xtb files.

Exports:
    rebrand_translations(xtbText, messages, replacements) -> dict:
        {
            'xml': str,
            'missing': list[str],
            'translated': int
        }
"""

from __future__ import annotations

import copy
import xml.etree.ElementTree as ET
from typing import Any, Dict, List, Mapping, Sequence, Tuple


def _apply_replacements(text: str | None, replacements: Sequence[Tuple[str, str]]) -> str | None:
    if not text:
        return text
    for old_str, new_str in replacements:
        text = text.replace(old_str, new_str)
    return text


def rebrand_translations(
    xtbText: str,
    messages: Sequence[Mapping[str, Any]],
    replacements: Mapping[str, str],
) -> Dict[str, Any]:
    """Rebrand translations from legacy IDs into current GRIT IDs.

    Args:
        xtbText: Raw XML content of a translation bundle (.xtb).
        messages: List of dicts, each with {'id': str, 'legacy_ids': list[str]}.
        replacements: Mapping of branding strings to replace, e.g. {'Chromium': 'Windrise Browser'}.

    Returns:
        dict with:
            'xml': Rebranded XML bundle text.
            'missing': List of current message IDs where no translation could be found.
            'translated': Count of messages with valid translations (existing or migrated).

    Raises:
        ValueError: If duplicate message IDs are supplied in messages or found in the XML.
    """
    # 1. Reject duplicate message IDs in input
    seen_msg_ids: set[str] = set()
    for m in messages:
        mid = str(m.get('id', ''))
        if not mid:
            continue
        if mid in seen_msg_ids:
            raise ValueError(f"Duplicate id in messages input: {mid}")
        seen_msg_ids.add(mid)

    # 2. Parse XML and detect duplicate IDs
    root = ET.fromstring(xtbText)
    seen_xtb_ids: set[str] = set()
    trans_map: dict[str, ET.Element] = {}

    for node in root.findall('translation'):
        nid = node.attrib.get('id')
        if not nid:
            continue
        if nid in seen_xtb_ids:
            raise ValueError(f"Duplicate translation id in XML: {nid}")
        seen_xtb_ids.add(nid)
        trans_map[nid] = node

    # 3. Sort replacements longest-first to avoid partial substring collisions
    sorted_replacements: list[tuple[str, str]] = sorted(
        replacements.items(), key=lambda kv: len(kv[0]), reverse=True
    )

    missing: list[str] = []
    translated = 0

    # 4. Process each message
    for m in messages:
        target_id = str(m.get('id', ''))
        if not target_id:
            continue

        # Existing new ID takes precedence; repeated run is idempotent
        if target_id in trans_map:
            translated += 1
            continue

        legacy_ids = [str(x) for x in m.get('legacy_ids', [])]
        found_source: ET.Element | None = None
        for lid in legacy_ids:
            if lid in trans_map:
                found_source = trans_map[lid]
                break

        if found_source is None:
            missing.append(target_id)
        else:
            # Deep clone the existing translation node
            new_node = copy.deepcopy(found_source)
            new_node.attrib['id'] = target_id

            # Apply replacements to text
            new_node.text = _apply_replacements(new_node.text, sorted_replacements)

            # Preserve placeholders and apply replacements to placeholder text/tail
            for child in new_node.iter():
                if child is not new_node:
                    child.text = _apply_replacements(child.text, sorted_replacements)
                child.tail = _apply_replacements(child.tail, sorted_replacements)

            # Append to root and index
            root.append(new_node)
            trans_map[target_id] = new_node
            translated += 1

    # 5. Serialize XML with standard translationbundle header
    body = ET.tostring(root, encoding='utf-8').decode('utf-8')
    header = '<?xml version="1.0" encoding="UTF-8"?>\n<!DOCTYPE translationbundle>\n'
    xml_out = header + body

    return {
        'xml': xml_out,
        'missing': missing,
        'translated': translated,
    }


if __name__ == '__main__':
    import sys
    print("xtb_rebrand module loaded successfully")
    sys.exit(0)
