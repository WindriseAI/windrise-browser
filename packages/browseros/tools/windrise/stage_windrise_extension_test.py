#!/usr/bin/env python3
"""Tests for stage_windrise_extension.py cryptographic and inventory checks."""

from __future__ import annotations

import os
import shutil
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import stage_windrise_extension as stager
from bos_build.lib.windrise_extension import _verify_rsa_sha256_signature

_EXT_DIR_ENV = os.environ.get("WINDRISE_EXTENSION_DIR", "").strip()
SIGNED_EXTENSION_DIR: Path | None = Path(_EXT_DIR_ENV) if _EXT_DIR_ENV else None


def _require_signed_extension_dir(test_case: unittest.TestCase) -> Path:
    if SIGNED_EXTENSION_DIR is None or not SIGNED_EXTENSION_DIR.is_dir():
        test_case.skipTest(
            "Signed desktop extension directory not configured (set WINDRISE_EXTENSION_DIR)"
        )
    assert SIGNED_EXTENSION_DIR is not None
    return SIGNED_EXTENSION_DIR


class StageWindriseExtensionTest(unittest.TestCase):
    def _copy_signed_tree(self, source_dir: Path, tmp: str) -> Path:
        dest = Path(tmp) / "windrise-extension"
        shutil.copytree(source_dir, dest)
        return dest

    def test_signed_desktop_extension_passes_when_present(self):
        source_dir = _require_signed_extension_dir(self)
        files = stager.verify_source_tree(source_dir)
        self.assertGreater(len(files), 10)

    def test_tampered_file_rejected_before_staging(self):
        source_dir = _require_signed_extension_dir(self)
        with tempfile.TemporaryDirectory() as tmp:
            tree = self._copy_signed_tree(source_dir, tmp)
            target = tree / "flash-card.js"
            target.write_text(target.read_text(encoding="utf-8") + "\n// tampered\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
                stager.verify_source_tree(tree)

    def test_extra_file_rejected(self):
        source_dir = _require_signed_extension_dir(self)
        with tempfile.TemporaryDirectory() as tmp:
            tree = self._copy_signed_tree(source_dir, tmp)
            (tree / "extra.js").write_text("console.log(1);\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Unsigned extra files"):
                stager.verify_source_tree(tree)

    def test_missing_file_rejected(self):
        source_dir = _require_signed_extension_dir(self)
        with tempfile.TemporaryDirectory() as tmp:
            tree = self._copy_signed_tree(source_dir, tmp)
            (tree / "flash-card.js").unlink()
            with self.assertRaisesRegex(ValueError, "Missing files listed"):
                stager.verify_source_tree(tree)

    def test_tampered_signature_rejected(self):
        source_dir = _require_signed_extension_dir(self)
        with tempfile.TemporaryDirectory() as tmp:
            tree = self._copy_signed_tree(source_dir, tmp)
            sig_path = tree / ".windrise-signature"
            raw = sig_path.read_text(encoding="utf-8").strip()
            flipped = ("A" if raw[0] != "A" else "B") + raw[1:]
            sig_path.write_text(flipped + "\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "RSA-SHA256 verification failed"):
                stager.verify_source_tree(tree)

    def test_node_fallback_rejects_and_accepts(self):
        source_dir = _require_signed_extension_dir(self)
        payload = (source_dir / ".windrise-integrity").read_bytes()
        import base64

        sig_bytes = base64.b64decode(
            (SIGNED_EXTENSION_DIR / ".windrise-signature").read_text(encoding="utf-8").strip()
        )
        orig_import = __import__

        def block_cryptography(name, *args, **kwargs):
            if name == "cryptography" or name.startswith("cryptography."):
                raise ImportError("blocked for fallback test")
            return orig_import(name, *args, **kwargs)

        with mock.patch("builtins.__import__", side_effect=block_cryptography):
            _verify_rsa_sha256_signature(payload, sig_bytes)
            with self.assertRaisesRegex(ValueError, "RSA-SHA256 verification failed"):
                _verify_rsa_sha256_signature(payload + b"x", sig_bytes)


if __name__ == "__main__":
    unittest.main()
