"""Verify the signed Windrise assistant against the release trust anchor."""

from __future__ import annotations

import base64
import binascii
import json
import re
import shutil
import subprocess
from collections.abc import Iterator
from pathlib import Path

from .browser_directory_zip import check_member_path, sha256_file

WINDRISE_ASSISTANT_EXTENSION_ID = "amlnokjigkbgngdgkofikfphjbknfodp"
WINDRISE_EXTENSION_PUBLIC_KEY = (
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu/BdI9vbyMwLGJs9imDubXmoawZD3wzoYTUB"
    "SIru4A4+KflzIz04Ps8JUFpUmSJdUkgedCujSxQFt+ktysIc6tZAo7xXDqW/wDHr3py5sX7rfAILXh8o"
    "8lG9fRbJZjBj38rb/PTMvmvRJMCKnmM9MAaPwt3DT2Nj8xzR08GzeWdf6mfThswAA5w0YRIlVMhExKBF"
    "fB3b6Nb969QPha5//pxZMBoPSXktb4Gemx3esOtVWoWpxFEbfYs2td1MXGMlvziBN96hZ+49TcnFRKd"
    "zJoALB0q9xp1DHPlM0n9EMNKY+Fps0/17GrwGCYhb3DSyWEX7MXQLZraiT2eWQxpiDwIDAQAB"
)
EXPECTED_PUBLIC_KEY = WINDRISE_EXTENSION_PUBLIC_KEY
INVENTORY_HEADER = "windrise-extension-v1"
# Match the limits in Chromium's Windrise directory verifier.
MAX_FILE_BYTES = 64 * 1024 * 1024
REQUIRED_FILES = {
    "manifest.json": MAX_FILE_BYTES,
    ".windrise-integrity": 1024 * 1024,
    ".windrise-signature": 1024,
}
_ALLOWED_REL_PATH_RE = re.compile(r"^[A-Za-z0-9_.\-/]+$")
_INVENTORY_ENTRY_RE = re.compile(r"^([0-9a-f]{64}) ([A-Za-z0-9_.\-/]+)$")
_NODE_VERIFY_SCRIPT = (
    'const crypto = require("node:crypto");'
    'const fs = require("node:fs");'
    "const payload = fs.readFileSync(0);"
    'const key = crypto.createPublicKey({key: Buffer.from(process.argv[1], "base64"), type: "spki", format: "der"});'
    'const sig = Buffer.from(process.argv[2], "base64");'
    'if (!crypto.verify("sha256", payload, key, sig)) process.exit(1);'
)


def _verify_rsa_sha256_signature(payload: bytes, signature_bytes: bytes) -> None:
    """Verify an RSA PKCS1v15 SHA-256 signature against EXPECTED_PUBLIC_KEY."""
    try:
        from cryptography.exceptions import InvalidSignature
        from cryptography.hazmat.primitives import hashes, serialization
        from cryptography.hazmat.primitives.asymmetric import padding, rsa
    except ImportError:
        node = shutil.which("node")
        if not node:
            raise ValueError(
                "Cannot verify .windrise-signature: neither Python 'cryptography' nor 'node' is available"
            )
        signature_b64 = base64.b64encode(signature_bytes).decode("ascii")
        result = subprocess.run(
            [node, "-e", _NODE_VERIFY_SCRIPT, EXPECTED_PUBLIC_KEY, signature_b64],
            input=payload,
            capture_output=True,
            check=False,
        )
        if result.returncode != 0:
            raise ValueError(".windrise-signature RSA-SHA256 verification failed")
        return

    public_der = base64.b64decode(EXPECTED_PUBLIC_KEY, validate=True)
    public_key = serialization.load_der_public_key(public_der)
    if not isinstance(public_key, rsa.RSAPublicKey):
        raise ValueError("Pinned Windrise extension public key is not an RSA key")
    try:
        public_key.verify(
            signature_bytes,
            payload,
            padding.PKCS1v15(),
            hashes.SHA256(),
        )
    except InvalidSignature as exc:
        raise ValueError(".windrise-signature RSA-SHA256 verification failed") from exc


def _parse_inventory(inventory_text: str) -> dict[str, str]:
    """Parse .windrise-integrity into a {relative_posix_path: sha256_hex} map."""
    if not inventory_text.endswith("\n"):
        raise ValueError(".windrise-integrity must end with a newline")
    lines = inventory_text.splitlines()
    if len(lines) < 3:
        raise ValueError(".windrise-integrity is truncated")
    if lines[0] != INVENTORY_HEADER or lines[1] != WINDRISE_ASSISTANT_EXTENSION_ID:
        raise ValueError(".windrise-integrity header does not match Windrise assistant identity")

    signed_hashes: dict[str, str] = {}
    for line in lines[2:]:
        match = _INVENTORY_ENTRY_RE.fullmatch(line)
        if not match:
            raise ValueError(f"Malformed .windrise-integrity entry: {line!r}")
        digest, rel_path = match.group(1), match.group(2)
        if rel_path in signed_hashes:
            raise ValueError(f"Duplicate entry in .windrise-integrity: {rel_path}")
        signed_hashes[rel_path] = digest
    return signed_hashes


def _raise_walk_error(error: OSError) -> None:
    raise error


def _extension_files(root: Path) -> Iterator[Path]:
    for directory, dirs, files in root.walk(on_error=_raise_walk_error):
        for name in dirs + files:
            entry = directory / name
            if entry.is_symlink() or entry.is_junction():
                raise ValueError(f"Links are forbidden inside signed extension: {entry}")
        for name in files:
            yield directory / name


def verify_source_tree(source_dir: Path) -> list[Path]:
    """Verify manifest, path safety, SHA-256 inventory, and RSA signature."""
    if source_dir.is_symlink() or source_dir.is_junction() or not source_dir.is_dir():
        raise ValueError(f"Signed extension directory not found: {source_dir}")

    for name, max_bytes in REQUIRED_FILES.items():
        path = source_dir / name
        if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= max_bytes:
            raise ValueError(f"Missing or invalid required file: {path}")

    manifest = json.loads((source_dir / "manifest.json").read_text(encoding="utf-8"))
    if not isinstance(manifest, dict) or manifest.get("key") != EXPECTED_PUBLIC_KEY:
        raise ValueError("manifest.json public key does not match Windrise assistant key")

    inventory_bytes = (source_dir / ".windrise-integrity").read_bytes()
    try:
        inventory_text = inventory_bytes.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError(".windrise-integrity is not valid UTF-8") from exc

    signature_text = (source_dir / ".windrise-signature").read_text(encoding="utf-8").strip()
    try:
        signature_bytes = base64.b64decode(signature_text, validate=True)
    except (binascii.Error, ValueError) as exc:
        raise ValueError(".windrise-signature is not valid base64") from exc
    _verify_rsa_sha256_signature(inventory_bytes, signature_bytes)

    signed_hashes = _parse_inventory(inventory_text)

    files_to_stage: list[Path] = []
    actual_rel_paths: list[str] = []
    seen_paths: set[str] = set()
    for entry in _extension_files(source_dir):
        if not entry.is_file():
            raise ValueError(f"Unsupported non-regular file inside signed extension: {entry}")

        files_to_stage.append(entry)
        rel = entry.relative_to(source_dir).as_posix()
        if rel in (".windrise-integrity", ".windrise-signature"):
            continue
        if entry.stat().st_size > MAX_FILE_BYTES:
            raise ValueError(f"Signed extension file exceeds runtime size limit: {rel}")
        if not _ALLOWED_REL_PATH_RE.fullmatch(rel):
            raise ValueError(f"Extension file path contains disallowed characters: {rel}")
        path_key = check_member_path(f"windrise_extensions/windrise-extension/{rel}")
        if path_key in seen_paths:
            raise ValueError(f"Duplicate Windows path in signed extension: {rel}")
        seen_paths.add(path_key)
        actual_rel_paths.append(rel)

    actual_hashes = {
        rel: sha256_file(source_dir / rel) for rel in sorted(actual_rel_paths)
    }

    missing = sorted(set(signed_hashes) - set(actual_hashes))
    if missing:
        raise ValueError(f"Missing files listed in .windrise-integrity: {', '.join(missing[:5])}")
    extra = sorted(set(actual_hashes) - set(signed_hashes))
    if extra:
        raise ValueError(f"Unsigned extra files not in .windrise-integrity: {', '.join(extra[:5])}")
    mismatched = [
        rel for rel in sorted(actual_hashes) if actual_hashes[rel] != signed_hashes[rel]
    ]
    if mismatched:
        raise ValueError(f"SHA-256 mismatch against .windrise-integrity: {', '.join(mismatched[:5])}")

    expected_inventory = "\n".join(
        [
            INVENTORY_HEADER,
            WINDRISE_ASSISTANT_EXTENSION_ID,
            *(f"{actual_hashes[rel]} {rel}" for rel in sorted(actual_hashes)),
        ]
    ) + "\n"
    if inventory_bytes != expected_inventory.encode("utf-8"):
        raise ValueError(".windrise-integrity ordering or formatting does not match canonical inventory")

    return files_to_stage


def verify_bundled_extension(root: Path, product_id: str) -> None:
    """Check the windrise assistant at the output or installer runtime root."""
    if product_id != "browseros":
        return
    bundles = root / "windrise_extensions"
    if bundles.is_symlink() or bundles.is_junction():
        raise ValueError(f"Links are forbidden in the Windrise extension path: {bundles}")
    verify_source_tree(bundles / "windrise-extension")
