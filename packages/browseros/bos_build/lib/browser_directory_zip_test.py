#!/usr/bin/env python3
"""Tests for browser directory ZIP layout and retired payload guards."""

import hashlib
import io
import json
import lzma
import struct
import tarfile
import tempfile
import unittest
import zipfile
from pathlib import Path

from .browser_directory_zip import (
    XZ_MAGIC,
    check_bundled_extensions_manifest,
    check_layout,
    create_directory_tar_xz,
    inspect_directory_archive,
    inspect_directory_tar_xz,
    inspect_directory_zip,
    tar_xz_filters,
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

    def test_tar_xz_filters_uses_x86_bcj_for_x86_and_x64_only(self) -> None:
        self.assertEqual(
            [f["id"] for f in tar_xz_filters("x64")],
            [lzma.FILTER_X86, lzma.FILTER_LZMA2],
        )
        self.assertEqual(
            [f["id"] for f in tar_xz_filters("x86")],
            [lzma.FILTER_X86, lzma.FILTER_LZMA2],
        )
        self.assertEqual(
            [f["id"] for f in tar_xz_filters("arm64")],
            [lzma.FILTER_LZMA2],
        )

    def test_create_and_inspect_directory_tar_xz_roundtrip(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "tree"
            (src / VERSION / "windrise_extensions").mkdir(parents=True)
            (src / "chrome.exe").write_bytes(_minimal_pe_x64())
            (src / VERSION / "chrome.dll").write_bytes(b"dll-payload")
            (src / VERSION / f"{VERSION}.manifest").write_bytes(b"<assembly/>")
            (src / VERSION / "windrise_extensions" / "bundled_extensions.json").write_bytes(b"{}\n")

            files = [
                (p, p.relative_to(src).as_posix())
                for p in sorted(src.rglob("*"))
                if p.is_file()
            ]
            tar_xz_path = root / "browser.tar.xz"
            create_directory_tar_xz(files, tar_xz_path, arch="x64", preset=1)

            raw = tar_xz_path.read_bytes()
            self.assertTrue(raw.startswith(XZ_MAGIC))
            digest = hashlib.sha256(raw).hexdigest()
            meta_path = root / "browser.tar.xz.json"
            meta_path.write_text(
                json.dumps(
                    {
                        "name": tar_xz_path.name,
                        "format": "tar.xz",
                        "platform": "win32",
                        "arch": "x64",
                        "version": VERSION,
                        "buildId": f"sha256-{digest}",
                        "size": len(raw),
                        "sha256": digest,
                    }
                ),
                encoding="utf-8",
            )

            result = inspect_directory_archive(tar_xz_path)
            self.assertEqual(result["format"], "tar.xz")
            self.assertEqual(result["executable"], "chrome.exe")
            self.assertEqual(result["files"], 4)

            # Verify normalized tar metadata (whitespace-safe bsdtar -tvf output).
            with tarfile.open(tar_xz_path, "r:xz") as archive:
                for member in archive.getmembers():
                    self.assertEqual(member.uid, 0)
                    self.assertEqual(member.gid, 0)
                    self.assertEqual(member.uname, "0")
                    self.assertEqual(member.gname, "0")

    def test_inspect_directory_tar_xz_rejects_retired_payloads_and_symlinks(self) -> None:
        cases = [
            (
                "non-empty bundled_extensions.json",
                {f"{VERSION}/windrise_extensions/bundled_extensions.json": b'{"x":1}'},
                "Non-empty bundled_extensions.json",
            ),
            (
                "retired crx",
                {f"{VERSION}/windrise_extensions/adlpneommgkgeanpaekgoaolcpncohkf.crx": b"crx"},
                "Bundled extension CRX is forbidden",
            ),
            (
                "BrowserOSServer",
                {f"{VERSION}/BrowserOSServer/browseros_server.exe": b"srv"},
                "Bundled upstream server payload is forbidden",
            ),
            (
                "stale version manifest",
                {f"{VERSION}/158.0.8127.18.manifest": b"<assembly/>"},
                "Mismatched assembly manifest",
            ),
        ]
        for label, extra_files, expected_err in cases:
            with self.subTest(case=label), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                tar_xz_path = root / "bad.tar.xz"
                members = {
                    "chrome.exe": _minimal_pe_x64(),
                    f"{VERSION}/chrome.dll": b"dll",
                    f"{VERSION}/{VERSION}.manifest": b"<assembly/>",
                    **extra_files,
                }
                with tarfile.open(tar_xz_path, "w:xz") as archive:
                    for rel, payload in members.items():
                        info = tarfile.TarInfo(name=rel)
                        info.size = len(payload)
                        archive.addfile(info, io.BytesIO(payload))
                raw = tar_xz_path.read_bytes()
                digest = hashlib.sha256(raw).hexdigest()
                meta_path = root / "bad.tar.xz.json"
                meta_path.write_text(
                    json.dumps(
                        {
                            "name": tar_xz_path.name,
                            "format": "tar.xz",
                            "platform": "win32",
                            "arch": "x64",
                            "version": VERSION,
                            "buildId": f"sha256-{digest}",
                            "size": len(raw),
                            "sha256": digest,
                        }
                    ),
                    encoding="utf-8",
                )
                with self.assertRaisesRegex(ValueError, expected_err):
                    inspect_directory_tar_xz(tar_xz_path, meta_path)

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            tar_xz_path = root / "symlink.tar.xz"
            with tarfile.open(tar_xz_path, "w:xz") as archive:
                exe = tarfile.TarInfo("chrome.exe")
                exe_bytes = _minimal_pe_x64()
                exe.size = len(exe_bytes)
                archive.addfile(exe, io.BytesIO(exe_bytes))
                link = tarfile.TarInfo(f"{VERSION}/evil")
                link.type = tarfile.SYMTYPE
                link.linkname = "../outside"
                archive.addfile(link)
            raw = tar_xz_path.read_bytes()
            digest = hashlib.sha256(raw).hexdigest()
            meta_path = root / "symlink.tar.xz.json"
            meta_path.write_text(
                json.dumps(
                    {
                        "name": tar_xz_path.name,
                        "format": "tar.xz",
                        "platform": "win32",
                        "arch": "x64",
                        "version": VERSION,
                        "buildId": f"sha256-{digest}",
                        "size": len(raw),
                        "sha256": digest,
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "Unsupported TAR.XZ link/special file"):
                inspect_directory_tar_xz(tar_xz_path, meta_path)

    def test_inspect_directory_tar_xz_rejects_wrong_format_and_non_xz_bytes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            fake_path = root / "fake.tar.xz"
            fake_bytes = b"PK\x03\x04not-an-xz-file"
            fake_path.write_bytes(fake_bytes)
            digest = hashlib.sha256(fake_bytes).hexdigest()
            meta_path = root / "fake.tar.xz.json"
            meta_path.write_text(
                json.dumps(
                    {
                        "name": fake_path.name,
                        "format": "zip",
                        "platform": "win32",
                        "arch": "x64",
                        "version": VERSION,
                        "buildId": f"sha256-{digest}",
                        "size": len(fake_bytes),
                        "sha256": digest,
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "Metadata name/format"):
                inspect_directory_tar_xz(fake_path, meta_path)

            meta_path.write_text(
                json.dumps(
                    {
                        "name": fake_path.name,
                        "format": "tar.xz",
                        "platform": "win32",
                        "arch": "x64",
                        "version": VERSION,
                        "buildId": f"sha256-{digest}",
                        "size": len(fake_bytes),
                        "sha256": digest,
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "XZ magic header"):
                inspect_directory_tar_xz(fake_path, meta_path)


if __name__ == "__main__":
    unittest.main()
