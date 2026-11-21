#!/usr/bin/env python3
"""Tests for the Windows packaging module (autoninja routing and validate())."""

import json
import struct
import tarfile
import tempfile
import unittest
import zipfile
from pathlib import Path
from types import SimpleNamespace
from typing import cast
from unittest import mock

from . import windows
from ..compile import standard
from ...core.context import ArtifactRegistry, Context
from ...core.products import get_product_descriptor
from ...core.step import ValidationError
from ...lib.browser_directory_zip import (
    XZ_MAGIC,
    inspect_directory_tar_xz,
    inspect_directory_zip,
)


class BuildMiniInstallerTest(unittest.TestCase):
    def test_routes_through_shared_argv_builder_with_override(self):
        ctx = cast(
            Context,
            SimpleNamespace(
                out_dir="out/Default_x64", chromium_src=Path("/tmp/chromium-src")
            ),
        )
        with (
            mock.patch.object(windows, "run_command") as run_cmd,
            mock.patch.object(standard, "IS_WINDOWS", return_value=False),
            mock.patch("os.chdir"),
            mock.patch("os.getcwd", return_value="/anywhere"),
            mock.patch.dict("os.environ", {"BROWSEROS_NINJA_JOBS": "8"}, clear=True),
        ):
            result = windows.build_mini_installer(ctx)
        run_cmd.assert_called_once_with(
            ["autoninja", "-C", "out/Default_x64", "-j", "8", "setup", "mini_installer"]
        )
        # Artifacts were never produced (run_command is mocked), so it reports failure.
        self.assertFalse(result)

    def test_unsigned_step_leaves_product_alias_for_packaging(self):
        for product_id in ("browseros", "browserclaw"):
            with self.subTest(product=product_id), tempfile.TemporaryDirectory() as tmp:
                product = get_product_descriptor(product_id)
                build_output_dir = Path(tmp) / "out" / "Default"
                build_output_dir.mkdir(parents=True)
                chrome_path = build_output_dir / "chrome.exe"
                product_path = build_output_dir / f"{product.app_base_name}.exe"
                chrome_path.write_bytes(b"chrome")
                ctx = cast(
                    Context,
                    SimpleNamespace(
                        chromium_src=Path(tmp),
                        out_dir="out/Default",
                        get_app_path=lambda: product_path,
                    ),
                )

                with mock.patch.object(
                    windows, "build_mini_installer", return_value=True
                ):
                    windows.MiniInstallerModule().execute(ctx)

                self.assertEqual(chrome_path.read_bytes(), b"chrome")
                self.assertFalse(product_path.exists())


class WindowsExecutableFinalizationTest(unittest.TestCase):
    def test_packages_final_signed_bytes_with_installer_runtime_tree(self):
        for product_id in ("browseros", "browserclaw"):
            with self.subTest(product=product_id), tempfile.TemporaryDirectory() as tmp:
                product = get_product_descriptor(product_id)
                root = Path(tmp)
                build_output_dir = root / "out" / "Default"
                browser_tree = (
                    build_output_dir / "gen/chrome/installer/mini_installer"
                    / "mini_installer/temp_installer_archive/Chrome-bin"
                )
                version = "158.0.8127.41"
                version_tree = browser_tree / version
                version_tree.mkdir(parents=True)
                (root / "chrome").mkdir()
                (root / "chrome/VERSION").write_text("MAJOR=158\nMINOR=0\nBUILD=8127\nPATCH=41\n")
                chrome_path = build_output_dir / "chrome.exe"
                product_path = build_output_dir / f"{product.app_base_name}.exe"
                executable = bytearray(70)
                executable[:2] = b"MZ"
                struct.pack_into("<I", executable, 60, 64)
                executable[64:68] = b"PE\0\0"
                struct.pack_into("<H", executable, 68, 0x8664)
                chrome_path.write_bytes(executable)
                (browser_tree / "chrome.exe").write_bytes(executable)
                (version_tree / "chrome.dll").write_bytes(b"runtime DLL")
                resource = version_tree / "windrise_extensions/bundled_extensions.json"
                resource.parent.mkdir(parents=True)
                resource.write_bytes(b"{}\n")
                (build_output_dir / "mini_installer.exe").write_bytes(b"signed installer")
                # Raw build-output profiles and unrelated files must not leak.
                profile = build_output_dir / "User Data/Default"
                profile.mkdir(parents=True)
                (profile / "Preferences").write_bytes(b"private")
                registry = ArtifactRegistry()
                ctx = cast(Context, SimpleNamespace(
                    chromium_src=root,
                    out_dir="out/Default",
                    architecture="x64",
                    artifact_registry=registry,
                    get_chromium_app_path=lambda: chrome_path,
                    get_app_path=lambda: product_path,
                    get_dist_dir=lambda: root / "dist",
                    get_artifact_name=lambda kind: product.artifact_filename(kind, "0.31.0", "x64"),
                ))
                windows.WindowsPackageModule().execute(ctx)

                self.assertEqual(product_path.read_bytes(), bytes(executable))
                zip_artifact = registry.get("browser_directory_zip")
                zip_meta_artifact = registry.get("browser_directory_metadata")
                tar_xz_artifact = registry.get("browser_directory_tar_xz")
                tar_xz_meta_artifact = registry.get("browser_directory_tar_xz_metadata")

                with zipfile.ZipFile(zip_artifact) as archive:
                    zip_names = archive.namelist()
                    self.assertEqual(archive.read("chrome.exe"), bytes(executable))
                    self.assertEqual(archive.read(f"{version}/chrome.dll"), b"runtime DLL")
                    self.assertEqual(archive.read(f"{version}/windrise_extensions/bundled_extensions.json"), b"{}\n")
                    self.assertNotIn("User Data/Default/Preferences", zip_names)
                    self.assertNotIn("mini_installer.exe", zip_names)
                metadata = json.loads(zip_meta_artifact.read_text())
                self.assertEqual(metadata["format"], "zip")
                self.assertEqual(metadata["version"], version)
                self.assertEqual(metadata["sha256"], windows.sha256_file(zip_artifact))
                self.assertEqual(metadata["size"], zip_artifact.stat().st_size)
                inspect_directory_zip(zip_artifact, zip_meta_artifact)

                self.assertTrue(tar_xz_artifact.name.endswith("_browser_directory.tar.xz"))
                self.assertTrue(tar_xz_artifact.read_bytes().startswith(XZ_MAGIC))
                with tarfile.open(tar_xz_artifact, "r:xz") as tar_archive:
                    tar_members = tar_archive.getmembers()
                    self.assertEqual([m.name for m in tar_members], zip_names)
                    self.assertEqual(
                        tar_archive.extractfile("chrome.exe").read(), bytes(executable)
                    )
                    self.assertEqual(
                        tar_archive.extractfile(f"{version}/chrome.dll").read(),
                        b"runtime DLL",
                    )
                    self.assertEqual(
                        tar_archive.extractfile(
                            f"{version}/windrise_extensions/bundled_extensions.json"
                        ).read(),
                        b"{}\n",
                    )
                tar_metadata = json.loads(tar_xz_meta_artifact.read_text())
                self.assertEqual(tar_metadata["name"], tar_xz_artifact.name)
                self.assertEqual(tar_metadata["format"], "tar.xz")
                self.assertEqual(tar_metadata["platform"], "win32")
                self.assertEqual(tar_metadata["arch"], "x64")
                self.assertEqual(tar_metadata["version"], version)
                self.assertEqual(
                    tar_metadata["sha256"], windows.sha256_file(tar_xz_artifact)
                )
                self.assertEqual(
                    tar_metadata["buildId"], f"sha256-{tar_metadata['sha256']}"
                )
                self.assertEqual(tar_metadata["size"], tar_xz_artifact.stat().st_size)
                inspect_directory_tar_xz(tar_xz_artifact, tar_xz_meta_artifact)

                with zipfile.ZipFile(registry.get("installer_zip")) as archive:
                    self.assertEqual(archive.read(ctx.get_artifact_name("installer")), b"signed installer")

    def test_directory_zip_rejects_retired_extension_and_stale_manifest(self):
        version = "158.0.8127.41"
        cases = (
            ("windrise_extensions/adlpneommgkgeanpaekgoaolcpncohkf.crx", b"crx"),
            ("windrise_extensions/bundled_extensions.json", b'{"adlpneommgkgeanpaekgoaolcpncohkf": {}}'),
            ("browseros_extensions/bundled_extensions.json", b"{}\n"),
            ("158.0.8127.18.manifest", b"<assembly/>"),
            ("BrowserOSServer/browseros_server.exe", b"server"),
        )
        for rel, content in cases:
            with self.subTest(file=rel), tempfile.TemporaryDirectory() as tmp:
                product = get_product_descriptor("browseros")
                root = Path(tmp)
                build_output_dir = root / "out" / "Default"
                browser_tree = (
                    build_output_dir / "gen/chrome/installer/mini_installer"
                    / "mini_installer/temp_installer_archive/Chrome-bin"
                )
                version_tree = browser_tree / version
                version_tree.mkdir(parents=True)
                (root / "chrome").mkdir()
                (root / "chrome/VERSION").write_text("MAJOR=158\nMINOR=0\nBUILD=8127\nPATCH=41\n")
                executable = bytearray(70)
                executable[:2] = b"MZ"
                struct.pack_into("<I", executable, 60, 64)
                executable[64:68] = b"PE\0\0"
                struct.pack_into("<H", executable, 68, 0x8664)
                chrome_path = build_output_dir / "chrome.exe"
                chrome_path.write_bytes(executable)
                (browser_tree / "chrome.exe").write_bytes(executable)
                (version_tree / "chrome.dll").write_bytes(b"runtime DLL")
                bad_file = version_tree / rel
                bad_file.parent.mkdir(parents=True, exist_ok=True)
                bad_file.write_bytes(content)
                (build_output_dir / "mini_installer.exe").write_bytes(b"signed installer")
                ctx = cast(Context, SimpleNamespace(
                    chromium_src=root,
                    out_dir="out/Default",
                    architecture="x64",
                    artifact_registry=ArtifactRegistry(),
                    get_chromium_app_path=lambda: chrome_path,
                    get_app_path=lambda: build_output_dir / f"{product.app_base_name}.exe",
                    get_dist_dir=lambda: root / "dist",
                    get_artifact_name=lambda kind: product.artifact_filename(kind, "0.31.0", "x64"),
                ))
                with self.assertRaises(ValueError):
                    windows.WindowsPackageModule().execute(ctx)
                dist_dir = root / "dist"
                self.assertFalse((dist_dir / product.artifact_filename("browser_directory_zip", "0.31.0", "x64")).exists())
                self.assertFalse((dist_dir / product.artifact_filename("browser_directory_tar_xz", "0.31.0", "x64")).exists())




class WindowsPackageModuleValidateTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.chromium_src = Path(self._tmp.name) / "chromium" / "src"
        self.out_dir = "out/Default_x64"
        self.build_output_dir = self.chromium_src / self.out_dir
        self.build_output_dir.mkdir(parents=True)
        self.ctx = cast(
            Context,
            SimpleNamespace(chromium_src=self.chromium_src, out_dir=self.out_dir),
        )

    def _touch_output(self, name: str) -> Path:
        path = self.build_output_dir / name
        path.write_text("")
        return path


    def test_validate_raises_when_mini_installer_missing(self):
        mini_installer_path = self.build_output_dir / "mini_installer.exe"

        with mock.patch.object(windows, "IS_WINDOWS", return_value=True):
            with self.assertRaises(ValidationError) as raised:
                windows.WindowsPackageModule().validate(self.ctx)

        self.assertEqual(
            str(raised.exception), f"mini_installer.exe not found: {mini_installer_path}"
        )

    def test_validate_raises_when_not_windows(self):
        with mock.patch.object(windows, "IS_WINDOWS", return_value=False):
            with self.assertRaisesRegex(
                ValidationError, "Windows packaging requires Windows"
            ):
                windows.WindowsPackageModule().validate(self.ctx)


if __name__ == "__main__":
    unittest.main()
