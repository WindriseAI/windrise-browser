#!/usr/bin/env python3
"""Windows packaging module for BrowserOS"""

import json
import shutil
import zipfile
from pathlib import Path
from ...core.step import Step, ValidationError, step
from ...core.context import Context
from ...lib.utils import (
    run_command,
    log_info,
    log_error,
    log_success,
    log_warning,
    join_paths,
    IS_WINDOWS,
)
from ..compile.standard import autoninja_command
from ...lib.browser_directory_zip import (
    VERSION_PATTERN,
    check_bundled_extensions_manifest,
    check_layout,
    executable_arch,
    sha256_file,
)
from ...lib.release_preflight import bundled_resource_versions


@step("mini_installer", phase="sign", platforms=("windows",), optional=True)
class MiniInstallerModule(Step):
    """Build mini_installer.exe without signing.

    The signed release flow builds mini_installer inside WindowsSignModule
    (sign binaries -> build installer -> sign installer). Unsigned CI builds
    skip sign_windows entirely, so this module provides the installer build
    step that package_windows requires.
    """

    produces = []
    requires = []
    description = "Build unsigned mini_installer.exe (CI builds without signing)"

    def validate(self, context: Context) -> None:
        if not IS_WINDOWS():
            raise ValidationError("mini_installer build requires Windows")

        args_file = context.get_gn_args_file()
        if not args_file.exists():
            raise ValidationError(
                f"Build not configured - args.gn not found: {args_file}"
            )

    def execute(self, context: Context) -> None:
        if not build_mini_installer(context):
            raise RuntimeError("Failed to build mini_installer")


@step("package_windows", phase="package", platforms=("windows",))
class WindowsPackageModule(Step):
    produces = ["installer", "installer_zip", "browser_directory_zip", "browser_directory_metadata"]
    requires = []
    description = "Create Windows installers and complete browser directory ZIP"

    def validate(self, context: Context) -> None:
        if not IS_WINDOWS():
            raise ValidationError("Windows packaging requires Windows")

        build_output_dir = join_paths(context.chromium_src, context.out_dir)
        mini_installer_path = build_output_dir / "mini_installer.exe"

        if not mini_installer_path.exists():
            raise ValidationError(f"mini_installer.exe not found: {mini_installer_path}")

    def execute(self, context: Context) -> None:
        log_info("\n📦 Creating Windows packages...")

        installer_path = self._create_installer(context)
        zip_path = self._create_portable_zip(context)
        directory_zip, directory_metadata = self._create_directory_zip(context)
        product_executable_path = self._copy_product_executable(context)

        context.artifact_registry.add("installer", installer_path)
        context.artifact_registry.add("installer_zip", zip_path)
        context.artifact_registry.add("browser_directory_zip", directory_zip)
        context.artifact_registry.add("browser_directory_metadata", directory_metadata)
        context.artifact_registry.add("built_app", product_executable_path)

        log_success("Windows packages created successfully")

    def _copy_product_executable(self, ctx: Context) -> Path:
        """Create the product-named executable after installer packaging."""
        chrome_path = ctx.get_chromium_app_path()
        product_path = ctx.get_app_path()
        if not chrome_path.exists():
            raise RuntimeError(
                f"Primary browser executable not found after packaging: {chrome_path}"
            )

        try:
            shutil.copy2(chrome_path, product_path)
        except Exception as e:
            raise RuntimeError(f"Failed to create product executable: {e}") from e

        log_success(f"Product executable created: {product_path.name}")
        return product_path

    def _create_installer(self, ctx: Context) -> Path:
        build_output_dir = join_paths(ctx.chromium_src, ctx.out_dir)
        mini_installer_path = build_output_dir / "mini_installer.exe"

        output_dir = ctx.get_dist_dir()
        output_dir.mkdir(parents=True, exist_ok=True)

        installer_name = ctx.get_artifact_name("installer")
        installer_path = output_dir / installer_name

        try:
            shutil.copy2(mini_installer_path, installer_path)
            log_success(f"Installer created: {installer_name}")
            return installer_path
        except Exception as e:
            raise RuntimeError(f"Failed to create installer: {e}")

    def _create_portable_zip(self, ctx: Context) -> Path:
        build_output_dir = join_paths(ctx.chromium_src, ctx.out_dir)
        mini_installer_path = build_output_dir / "mini_installer.exe"

        output_dir = ctx.get_dist_dir()
        output_dir.mkdir(parents=True, exist_ok=True)

        zip_name = ctx.get_artifact_name("installer_zip")
        zip_path = output_dir / zip_name

        try:
            with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zipf:
                installer_name = ctx.get_artifact_name("installer")
                zipf.write(mini_installer_path, installer_name)

                file_size = mini_installer_path.stat().st_size
                log_info(f"Added installer to ZIP ({file_size // (1024*1024)} MB)")

            log_success(f"Installer ZIP created: {zip_name}")
            return zip_path
        except Exception as e:
            raise RuntimeError(f"Failed to create installer ZIP: {e}")

    def _create_directory_zip(self, ctx: Context) -> tuple[Path, Path]:
        """Reuse the mini_installer's release-manifest staging tree after signing.

        Upstream BUILD.gn passes $target_gen_dir/mini_installer as staging_dir;
        create_installer_archive.py adds temp_installer_archive/Chrome-bin.
        Strip only Chrome-bin, retaining every runtime file and version path.
        Do not use the raw build directory, which can contain user profiles.
        """
        build_output_dir = join_paths(ctx.chromium_src, ctx.out_dir)
        browser_tree = (
            build_output_dir / "gen" / "chrome" / "installer" / "mini_installer"
            / "mini_installer" / "temp_installer_archive" / "Chrome-bin"
        )
        if not browser_tree.is_dir():
            raise RuntimeError(f"Installer runtime tree is missing: {browser_tree}; build mini_installer first")

        # This is the same VERSION file BuildVersion() uses upstream, including
        # the product build offset; CHROMIUM_VERSION is only the upstream pin.
        version_parts = {}
        for line in (ctx.chromium_src / "chrome" / "VERSION").read_text(encoding="utf-8").splitlines():
            key, separator, value = line.partition("=")
            if separator:
                version_parts[key.strip()] = value.strip()
        version = ".".join(version_parts.get(key, "") for key in ("MAJOR", "MINOR", "BUILD", "PATCH"))
        if not VERSION_PATTERN.fullmatch(version):
            raise RuntimeError("chrome/VERSION does not contain a four-part Chromium version")

        entries = []
        files = []
        for path in sorted(browser_tree.rglob("*")):
            if path.is_symlink() or path.is_junction():
                raise RuntimeError(f"Links are forbidden in the installer runtime tree: {path}")
            relative = path.relative_to(browser_tree).as_posix()
            if path.is_dir():
                entries.append((relative + "/", True, 0))
            elif path.is_file():
                entries.append((relative, False, path.stat().st_size))
                files.append((path, relative))
            else:
                raise RuntimeError(f"Unsupported installer runtime entry: {path}")
        check_layout(entries, version)
        for path, relative in files:
            if path.name.casefold() == "bundled_extensions.json":
                check_bundled_extensions_manifest(path.read_bytes(), relative)

        staged_chrome = browser_tree / "chrome.exe"
        if sha256_file(staged_chrome) != sha256_file(build_output_dir / "chrome.exe"):
            raise RuntimeError("Installer tree chrome.exe differs from final build output; rebuild mini_installer after signing")
        with staged_chrome.open("rb") as source:
            arch = executable_arch(source)
        if arch != ctx.architecture:
            raise RuntimeError(f"Installer chrome.exe architecture {arch} differs from target {ctx.architecture}")

        output_dir = ctx.get_dist_dir()
        output_dir.mkdir(parents=True, exist_ok=True)
        zip_path = output_dir / ctx.get_artifact_name("browser_directory_zip")
        metadata_path = zip_path.with_suffix(zip_path.suffix + ".json")
        pending_zip = zip_path.with_suffix(zip_path.suffix + ".tmp")
        pending_metadata = metadata_path.with_suffix(metadata_path.suffix + ".tmp")
        resource_versions = bundled_resource_versions(ctx)
        try:
            with zipfile.ZipFile(pending_zip, "w", zipfile.ZIP_DEFLATED) as archive:
                for path, relative in files:
                    archive.write(path, relative)
            digest = sha256_file(pending_zip)
            metadata = {
                "name": zip_path.name,
                "format": "zip",
                "platform": "win32",
                "arch": arch,
                "version": version,
                "buildId": f"sha256-{digest}",
                "size": pending_zip.stat().st_size,
                "sha256": digest,
                "component_versions": resource_versions,
            }
            pending_metadata.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
            pending_zip.replace(zip_path)
            pending_metadata.replace(metadata_path)
        finally:
            pending_zip.unlink(missing_ok=True)
            pending_metadata.unlink(missing_ok=True)
        log_success(f"Browser directory ZIP created: {zip_path.name}")
        return zip_path, metadata_path



def build_mini_installer(ctx: Context) -> bool:
    """Build the mini_installer target if it doesn't exist"""
    log_info("\n🔨 Checking mini_installer build...")

    build_output_dir = join_paths(ctx.chromium_src, ctx.out_dir)
    mini_installer_path = build_output_dir / "mini_installer.exe"
    setup_exe_path = build_output_dir / "setup.exe"

    if mini_installer_path.exists() and setup_exe_path.exists():
        log_info(
            "mini_installer.exe and setup.exe already exist; rebuilding to ensure freshness"
        )
    elif setup_exe_path.exists() and not mini_installer_path.exists():
        log_info("setup.exe exists but mini_installer.exe missing")
    elif mini_installer_path.exists() and not setup_exe_path.exists():
        log_info("mini_installer.exe exists but setup.exe missing")

    log_info("Building setup and mini_installer targets...")

    try:
        cmd = autoninja_command(ctx.out_dir, ["setup", "mini_installer"])

        import os

        old_cwd = os.getcwd()
        os.chdir(ctx.chromium_src)

        try:
            run_command(cmd)
        finally:
            os.chdir(old_cwd)

        missing_artifacts = []
        if not setup_exe_path.exists():
            missing_artifacts.append("setup.exe")
        if not mini_installer_path.exists():
            missing_artifacts.append("mini_installer.exe")

        if not missing_artifacts:
            log_success("mini_installer and setup built successfully")
            return True

        log_error(
            "Build completed but missing artifacts: "
            + ", ".join(missing_artifacts)
        )
        return False

    except Exception as e:
        log_error(f"Failed to build setup/mini_installer: {e}")
        return False


def create_installer(ctx: Context) -> bool:
    """Create Windows installer (mini_installer.exe)"""
    log_info("\n🔧 Creating Windows installer...")

    build_output_dir = join_paths(ctx.chromium_src, ctx.out_dir)
    mini_installer_path = build_output_dir / "mini_installer.exe"

    if not mini_installer_path.exists():
        log_warning(f"mini_installer.exe not found at: {mini_installer_path}")
        log_info(f"To build the installer, run: autoninja -C {ctx.out_dir} mini_installer")
        return False

    output_dir = ctx.get_dist_dir()
    output_dir.mkdir(parents=True, exist_ok=True)

    installer_name = ctx.get_artifact_name("installer")
    installer_path = output_dir / installer_name

    try:
        shutil.copy2(mini_installer_path, installer_path)
        log_success(f"Installer created: {installer_name}")
        return True
    except Exception as e:
        log_error(f"Failed to create installer: {e}")
        return False


def create_portable_zip(ctx: Context) -> bool:
    """Create ZIP of just the installer for easier distribution"""
    log_info("\n📦 Creating installer ZIP package...")

    build_output_dir = join_paths(ctx.chromium_src, ctx.out_dir)
    mini_installer_path = build_output_dir / "mini_installer.exe"

    if not mini_installer_path.exists():
        log_warning(f"mini_installer.exe not found at: {mini_installer_path}")
        log_info(f"To build the installer, run: autoninja -C {ctx.out_dir} mini_installer")
        return False

    output_dir = ctx.get_dist_dir()
    output_dir.mkdir(parents=True, exist_ok=True)

    zip_name = ctx.get_artifact_name("installer_zip")
    zip_path = output_dir / zip_name

    try:
        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zipf:
            installer_name = ctx.get_artifact_name("installer")
            zipf.write(mini_installer_path, installer_name)

            file_size = mini_installer_path.stat().st_size
            log_info(f"Added installer to ZIP ({file_size // (1024*1024)} MB)")

        log_success(f"Installer ZIP created: {zip_name}")
        return True
    except Exception as e:
        log_error(f"Failed to create installer ZIP: {e}")
        return False


def get_target_cpu(build_output_dir: Path) -> str:
    """Get target CPU architecture from build configuration"""
    args_gn_path = build_output_dir / "args.gn"

    if not args_gn_path.exists():
        return "x64"  # Default

    try:
        args_gn_content = args_gn_path.read_text(encoding="utf-8")
        for cpu in ("x64", "x86", "arm64"):
            if f'target_cpu="{cpu}"' in args_gn_content:
                return cpu
    except Exception:
        pass

    return "x64"  # Default


def create_files_cfg_package(ctx: Context) -> bool:
    """Create package using Chromium's FILES.cfg approach (alternative method)"""
    log_info("\n📦 Creating FILES.cfg-based package...")

    files_cfg_path = (
        ctx.chromium_src / "chrome" / "tools" / "build" / "win" / "FILES.cfg"
    )

    if not files_cfg_path.exists():
        log_error(f"FILES.cfg not found at: {files_cfg_path}")
        return False

    # This would require implementing the filescfg module functionality
    # from ungoogled-chromium, which is quite complex
    log_warning("FILES.cfg packaging not yet implemented")
    return False
