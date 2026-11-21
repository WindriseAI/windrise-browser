#!/usr/bin/env python3
"""Windows directory ZIP contract and read-only release inspection (stdlib only).

Usage: python browser_directory_zip.py BROWSER.zip [--metadata BROWSER.zip.json]
This command never extracts files, starts a browser, or changes either input.
"""

import argparse
import hashlib
import json
import re
import stat
import struct
import sys
import zipfile
from pathlib import Path, PurePosixPath


VERSION_PATTERN = re.compile(r"\d+\.\d+\.\d+\.\d+")
BUILD_ID_PATTERN = re.compile(r"[A-Za-z0-9._-]{1,128}")
PE_ARCHITECTURES = {0x014C: "x86", 0x8664: "x64", 0xAA64: "arm64"}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def executable_arch(source) -> str:
    """Read PE machine type without loading or executing the binary."""
    header = source.read(64)
    if len(header) != 64 or header[:2] != b"MZ":
        raise ValueError("chrome.exe is not a PE executable")
    offset = struct.unpack_from("<I", header, 60)[0]
    if offset < 64 or offset > 1024 * 1024:
        raise ValueError("chrome.exe has an invalid PE header offset")
    source.seek(offset)
    pe_header = source.read(6)
    if len(pe_header) != 6 or pe_header[:4] != b"PE\0\0":
        raise ValueError("chrome.exe has an invalid PE header")
    machine = struct.unpack_from("<H", pe_header, 4)[0]
    if machine not in PE_ARCHITECTURES:
        raise ValueError(f"Unsupported chrome.exe machine type: {machine:#x}")
    return PE_ARCHITECTURES[machine]


def check_member_path(name: str) -> str:
    """Reject unsafe Windows paths and profile data; preserve server/default."""
    path = PurePosixPath(name)
    parts = name.rstrip("/").split("/")
    if (not name or "\\" in name or path.is_absolute()
            or any(part in ("", ".", "..") or ":" in part
                   or part.rstrip(" .") != part for part in parts)):
        raise ValueError(f"Unsafe ZIP member path: {name}")
    lowered = [part.casefold() for part in parts]
    for index, part in enumerate(lowered):
        if part in {"browserosserver", "browserclawserver"} or part == "browseros_server.exe":
            raise ValueError(f"Bundled upstream server payload is forbidden in browser ZIP: {name}")
        if (part in {"user data", "userdata", "guest profile", "system profile",
                     "local state", "preferences", "secure preferences", "login data",
                     "cookies", "history", "web data", "bookmarks"}
                or re.fullmatch(r"profile \d+", part)
                or part.startswith("singleton")
                or part == "default"):
            raise ValueError(f"User profile data is forbidden in browser ZIP: {name}")
    return "/".join(lowered)


def check_bundled_extensions_manifest(raw: bytes, label: str) -> None:
    """Reject missing, malformed, or non-empty bundled_extensions.json."""
    try:
        data = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ValueError(f"Invalid bundled_extensions.json in browser ZIP: {label}") from exc
    if not isinstance(data, dict) or data:
        raise ValueError(
            f"Non-empty bundled_extensions.json is forbidden in browser ZIP: {label}"
        )


def check_layout(entries: list[tuple[str, bool, int]], version: str) -> str:
    if not isinstance(version, str) or not VERSION_PATTERN.fullmatch(version):
        raise ValueError("version must be the actual four-part Chromium version")
    members = {}
    raw_names = {}
    for name, is_dir, size in entries:
        key = check_member_path(name)
        if key in members:
            raise ValueError(f"Duplicate Windows ZIP path: {name}")
        members[key] = (is_dir, size)
        raw_names[key] = name
    roots = [root for root in ("", "application/")
             if root + "chrome.exe" in members
             and not members[root + "chrome.exe"][0]
             and members[root + "chrome.exe"][1] > 0]
    if len(roots) != 1:
        raise ValueError("Expected exactly one chrome.exe at ZIP root or Application/chrome.exe")
    root = roots[0]
    versions = {key[len(root):].split("/", 1)[0] for key in members
                if key.startswith(root) and "/" in key[len(root):]
                and VERSION_PATTERN.fullmatch(key[len(root):].split("/", 1)[0])}
    if versions != {version}:
        raise ValueError(f"Version directories {sorted(versions)} do not match {version}")
    dll = members.get(f"{root}{version}/chrome.dll")
    if not dll or dll[0] or dll[1] <= 0:
        raise ValueError(f"Missing nonempty {root}{version}/chrome.dll")
    version_dir = f"{root}{version}"
    expected_manifest = f"{version}.manifest".casefold()
    for key, (is_dir, size) in members.items():
        if is_dir:
            continue
        posix = PurePosixPath(key)
        original = raw_names[key]
        if "browseros_extensions" in posix.parts:
            raise ValueError(f"Legacy browseros_extensions directory is forbidden in browser ZIP: {original}")
        if posix.parent.name in {"windrise_extensions", "browseros_extensions"} and posix.suffix == ".crx":
            raise ValueError(f"Bundled extension CRX is forbidden in browser ZIP: {original}")
        if posix.name == "bundled_extensions.json" and (size < 2 or size > 6):
            raise ValueError(
                f"Non-empty bundled_extensions.json is forbidden in browser ZIP: {original}"
            )
        if posix.suffix == ".manifest":
            in_version_dir = posix.parent.as_posix() == version_dir
            if (in_version_dir or VERSION_PATTERN.fullmatch(posix.stem)) and posix.name != expected_manifest:
                raise ValueError(
                    f"Mismatched assembly manifest in browser ZIP: {original} (expected {version}.manifest)"
                )
    return "Application/chrome.exe" if root else "chrome.exe"


def inspect_directory_zip(zip_path: Path, metadata_path: Path | None = None) -> dict:
    metadata_path = metadata_path or zip_path.with_suffix(zip_path.suffix + ".json")
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    if not isinstance(metadata, dict):
        raise ValueError("Metadata must be a JSON object")
    if metadata.get("name") != zip_path.name or metadata.get("format") != "zip":
        raise ValueError("Metadata name/format does not identify this directory ZIP")
    build_id = metadata.get("buildId")
    if not isinstance(build_id, str) or not BUILD_ID_PATTERN.fullmatch(build_id):
        raise ValueError("Metadata buildId is missing or invalid")
    if metadata.get("platform") != "win32" or metadata.get("arch") not in PE_ARCHITECTURES.values():
        raise ValueError("Metadata platform/arch is invalid")
    size = metadata.get("size")
    if type(size) is not int or size <= 0 or size != zip_path.stat().st_size:
        raise ValueError("Metadata size does not match ZIP byte count")
    digest = metadata.get("sha256")
    if not isinstance(digest, str) or not re.fullmatch(r"[a-f0-9]{64}", digest):
        raise ValueError("Metadata sha256 must be 64 lowercase hexadecimal characters")
    if sha256_file(zip_path) != digest:
        raise ValueError("Metadata sha256 does not match ZIP bytes")
    if build_id != f"sha256-{digest}":
        raise ValueError("Metadata buildId does not identify this ZIP's content")
    with zipfile.ZipFile(zip_path, "r") as archive:
        infos = archive.infolist()
        for info in infos:
            mode = info.external_attr >> 16
            if stat.S_ISLNK(mode) or (stat.S_IFMT(mode) not in (0, stat.S_IFREG, stat.S_IFDIR)):
                raise ValueError(f"Unsupported ZIP link/special file: {info.filename}")
        executable = check_layout(
            [(info.filename, info.is_dir(), info.file_size) for info in infos],
            metadata.get("version"),
        )
        for info in infos:
            if not info.is_dir() and PurePosixPath(info.filename).name.casefold() == "bundled_extensions.json":
                with archive.open(info) as manifest_file:
                    check_bundled_extensions_manifest(manifest_file.read(), info.filename)
        # Layout matching is case insensitive, as on the target filesystem.
        executable_info = next(info for info in infos if info.filename.casefold() == executable.casefold())
        with archive.open(executable_info) as source:
            if executable_arch(source) != metadata["arch"]:
                raise ValueError("Metadata arch does not match chrome.exe PE machine type")
        bad_member = archive.testzip()
        if bad_member:
            raise ValueError(f"ZIP CRC check failed: {bad_member}")
    return {**metadata, "executable": executable, "files": sum(not info.is_dir() for info in infos)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zip", type=Path)
    parser.add_argument("--metadata", type=Path)
    args = parser.parse_args()
    try:
        result = inspect_directory_zip(args.zip, args.metadata)
    except (OSError, ValueError, KeyError, RuntimeError, zipfile.BadZipFile, EOFError) as error:
        print(f"Directory ZIP check failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
