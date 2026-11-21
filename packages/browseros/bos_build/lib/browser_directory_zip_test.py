#!/usr/bin/env python3
"""Tests for browser directory ZIP layout and retired payload guards."""

import hashlib
import json
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path

from .browser_directory_zip import (
    check_bundled_extensions_manifest,
    check_layout,
    inspect_directory_zip,
)

VERSION = "158.0.8127.41"


def _minimal_pe_x64() -> bytes:
    buf = bytearray(70)
    buf[:2] = b"MZ"
    struct.pack_into("<I", buf, 60, 64)
    buf[64:68] = b"PE\0\0"
    struct.pack_into("<H", buf, 68, 0x8664)
    return bytes(buf)


def _base_entries() -> list[tuple[str, bool, int]]:
    return [
        ("chrome.exe", False, 70),
        (f"{VERSION}/", True, 0),
        (f"{VERSION}/chrome.dll", False, 128),
        (f"{VERSION}/{VERSION}.manifest", False, 64),
        (f"{VERSION}/windrise_extensions/", True, 0),
        (f"{VERSION}/windrise_extensions/bundled_extensions.json", False, 3),
    ]


class BrowserDirectoryZipLayoutTest(unittest.TestCase):
    def test_accepts_empty_bundled_manifest_and_matching_version_manifest(self) -> None:
        self.assertEqual(check_layout(_base_entries(), VERSION), "chrome.exe")

    def test_rejects_legacy_browseros_extensions_dir(self) -> None:
        entries = _base_entries() + [
            (f"{VERSION}/browseros_extensions/bundled_extensions.json", False, 3),
        ]
        with self.assertRaisesRegex(ValueError, "Legacy browseros_extensions directory is forbidden"):
            check_layout(entries, VERSION)

    def test_rejects_bundled_extension_crx(self) -> None:
        entries = _base_entries() + [
            (
                f"{VERSION}/windrise_extensions/adlpneommgkgeanpaekgoaolcpncohkf.crx",
                False,
                104327,
            )
        ]
        with self.assertRaisesRegex(ValueError, "Bundled extension CRX is forbidden"):
            check_layout(entries, VERSION)

    def test_rejects_bundled_upstream_server(self) -> None:
        entries = _base_entries() + [
            (
                f"{VERSION}/BrowserOSServer/default/resources/bin/browseros_server.exe",
                False,
                1024,
            )
        ]
        with self.assertRaisesRegex(ValueError, "Bundled upstream server payload is forbidden"):
            check_layout(entries, VERSION)

    def test_rejects_mismatched_version_manifest(self) -> None:
        entries = _base_entries() + [
            (f"{VERSION}/158.0.8127.18.manifest", False, 64),
        ]
        with self.assertRaisesRegex(ValueError, "Mismatched assembly manifest"):
            check_layout(entries, VERSION)

    def test_rejects_nonempty_bundled_extensions_json_in_layout_and_bytes(self) -> None:
        entries = [
            entry
            for entry in _base_entries()
            if not entry[0].endswith("bundled_extensions.json")
        ] + [(f"{VERSION}/windrise_extensions/bundled_extensions.json", False, 149)]
        with self.assertRaisesRegex(ValueError, "Non-empty bundled_extensions.json"):
            check_layout(entries, VERSION)
        with self.assertRaisesRegex(ValueError, "Non-empty bundled_extensions.json"):
            check_bundled_extensions_manifest(
                b'{"adlpneommgkgeanpaekgoaolcpncohkf": {"external_version": "57.0.0.0"}}',
                "bundled_extensions.json",
            )

    def test_inspect_directory_zip_validates_bundled_extensions_json_content(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            zip_path = root / "browser.zip"
            with zipfile.ZipFile(zip_path, "w") as archive:
                archive.writestr("chrome.exe", _minimal_pe_x64())
                archive.writestr(f"{VERSION}/chrome.dll", b"dll")
                archive.writestr(f"{VERSION}/{VERSION}.manifest", b"<assembly/>")
                archive.writestr(
                    f"{VERSION}/windrise_extensions/bundled_extensions.json",
                    b'{"x":1}',
                )
            digest = hashlib.sha256(zip_path.read_bytes()).hexdigest()
            meta_path = root / "browser.zip.json"
            meta_path.write_text(
                json.dumps(
                    {
                        "name": zip_path.name,
                        "format": "zip",
                        "platform": "win32",
                        "arch": "x64",
                        "version": VERSION,
                        "buildId": f"sha256-{digest}",
                        "size": zip_path.stat().st_size,
                        "sha256": digest,
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "Non-empty bundled_extensions.json"):
                inspect_directory_zip(zip_path, meta_path)


if __name__ == "__main__":
    unittest.main()
