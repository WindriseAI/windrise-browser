#!/usr/bin/env python3
"""Windrise — the flagship product (BrowserOS rebrand).

Product id stays "browseros" so every existing patch, GN arg and resource
path keeps working; only the shipped identity is Windrise.
"""

from pathlib import Path

from ...core.products import (
    MacProductIdentity,
    ProductDescriptor,
)
from ..server_binaries import ServerBundle, SignSpec

BROWSEROS_PRODUCT = ProductDescriptor.define(
    id="browseros",
    display_name="Windrise Browser",
    windows_installer_guid="{4ADA8188-62A6-4C43-8FFC-1A2129C174CE}",
    summary="The open source agentic browser",
    description="Windrise is a privacy-focused web browser built on Chromium.",
    company="Windrise",
    linux_name="windrise-browser",
    # Windrise ships no upstream extensions; its assistant is supplied locally
    # by Windrise Bridge. The bundled MCP server does not depend on a CRX.
    required_extensions=(),
    # Reverse-DNS identity is si.windrise.browser, not the derived
    # si.windrise.Windrise: bundle ids are case-insensitive and read as a
    # domain, so the label must stay lowercase.
    mac=MacProductIdentity(
        bundle_id="si.windrise.browser",
        dev_bundle_id="si.windrise.browser.dev",
        signing_identifier="si.windrise.browser",
        dev_signing_identifier="si.windrise.browser.dev",
        framework_name="Windrise Framework.framework",
        dev_framework_name="Windrise Dev Framework.framework",
        dmg_volume_name="Windrise",
    ),
)

BROWSEROS_SERVER_BUNDLE = ServerBundle(
    id="browseros-server",
    name="Windrise Server",
    # Windrise Bridge communicates with the browser directly over pipe/extension
    # and does not ship the upstream BrowserOSServer sidecar.
    product_ids=(),
    chromium_output_root="BrowserOSServer",
    local_resources_root=Path("resources/binaries/browseros_server"),
    chromium_resources_root=Path("chrome/browser/browseros/server/resources"),
    macos_bundle_resources_root=Path(
        "Contents/Resources/BrowserOSServer/default/resources"
    ),
    windows_bundle_resources_root=Path("BrowserOSServer/default/resources"),
    macos_binaries={
        "browseros_server": SignSpec(
            "browseros_server", "runtime", "browseros-executable-entitlements.plist"
        ),
        "bun": SignSpec("bun", "runtime", "browseros-executable-entitlements.plist"),
        "rg": SignSpec("rg", "runtime"),
    },
    windows_binaries=("browseros_server.exe",),
    source_builder="bun",
    source_component="server",
    runtime_binary_name="browseros_server",
    required_in_chromium_output=False,
)
