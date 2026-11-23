#!/usr/bin/env python3
"""Tests for stage_windrise_extension.py cryptographic and inventory checks."""

from __future__ import annotations

import shutil
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import stage_windrise_extension as stager
from bos_build.lib.windrise_extension import _verify_rsa_sha256_signature

SIGNED_EXTENSION_DIR = Path(
    "F:/worktrees/bridge-replica-e45/desktop-integ-1123/electron/windrise-extension"
)


class StageWindriseExtensionTest(unittest.TestCase):
    def _copy_signed_tree(self, tmp: str) -> Path:
        dest = Path(tmp) / "windrise-extension"
        shutil.copytree(SIGNED_EXTENSION_DIR, dest)
        return dest

    def test_signed_desktop_extension_passes_when_present(self):
        if not SIGNED_EXTENSION_DIR.is_dir():
            self.skipTest("Signed desktop extension directory not present on host")
        files = stager.verify_source_tree(SIGNED_EXTENSION_DIR)
        self.assertGreater(len(files), 10)

    def test_tampered_file_rejected_before_staging(self):
        if not SIGNED_EXTENSION_DIR.is_dir():
            self.skipTest("Signed desktop extension directory not present on host")
        with tempfile.TemporaryDirectory() as tmp:
            tree = self._copy_signed_tree(tmp)
            target = tree / "flash-card.js"
            target.write_text(target.read_text(encoding="utf-8") + "\n// tampered\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
                stager.verify_source_tree(tree)

    def test_extra_file_rejected(self):
        if not SIGNED_EXTENSION_DIR.is_dir():
            self.skipTest("Signed desktop extension directory not present on host")
        with tempfile.TemporaryDirectory() as tmp:
            tree = self._copy_signed_tree(tmp)
            (tree / "extra.js").write_text("console.log(1);\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Unsigned extra files"):
                stager.verify_source_tree(tree)

    def test_missing_file_rejected(self):
        if not SIGNED_EXTENSION_DIR.is_dir():
            self.skipTest("Signed desktop extension directory not present on host")
        with tempfile.TemporaryDirectory() as tmp:
            tree = self._copy_signed_tree(tmp)
            (tree / "flash-card.js").unlink()
            with self.assertRaisesRegex(ValueError, "Missing files listed"):
                stager.verify_source_tree(tree)

    def test_tampered_signature_rejected(self):
        if not SIGNED_EXTENSION_DIR.is_dir():
            self.skipTest("Signed desktop extension directory not present on host")
        with tempfile.TemporaryDirectory() as tmp:
            tree = self._copy_signed_tree(tmp)
            sig_path = tree / ".windrise-signature"
            raw = sig_path.read_text(encoding="utf-8").strip()
            flipped = ("A" if raw[0] != "A" else "B") + raw[1:]
            sig_path.write_text(flipped + "\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "RSA-SHA256 verification failed"):
                stager.verify_source_tree(tree)

    def test_node_fallback_rejects_and_accepts(self):
        if not SIGNED_EXTENSION_DIR.is_dir():
            self.skipTest("Signed desktop extension directory not present on host")
        payload = (SIGNED_EXTENSION_DIR / ".windrise-integrity").read_bytes()
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
