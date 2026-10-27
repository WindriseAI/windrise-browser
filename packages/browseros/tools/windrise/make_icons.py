#!/usr/bin/env python3
"""Generate the Windrise product icon set from the Windrise Bridge brand art.

Replaces every raster icon under ``resources/browseros/icons`` with a Windrise
version at the same path and the same pixel size, so the Chromium overlay copy
step (``bos_build/config/copy_resources.yaml``) needs no changes.

Source art lives next to this script in ``brand-src/`` — the checked-in copy of
the brand export, so the build never reads a directory outside the repo:

* ``windrise-bridge-icon-1024.png``        — full art, used at 33 px and above
* ``windrise-bridge-icon-small-1024.png``  — redrawn silhouette, 32 px and below
* ``windrise-bridge-mark-mono-1024.png``   — flat mark, no plate; recolored white
* ``windrise-bridge-icon.svg``             — vector master, copied as-is for the
  two ``product_logo*.svg`` slots

The masters are already rasterized, so this script needs no SVG renderer and
stays reproducible with Pillow only.

macOS ``.icns`` blobs are rebuilt here as PNG-based ICNS containers, which
macOS 10.7+ reads; that covers the app and document icons on every supported
release. ``Assets.car`` is an Apple asset catalog that only ``actool`` (macOS +
Xcode) can compile, so regenerate it on a Mac from the ``Assets.xcassets`` PNGs
this script updates.

Usage:
    python packages/browseros/tools/windrise/make_icons.py
    python packages/browseros/tools/windrise/make_icons.py --brand-dir D:/brand-src
    python packages/browseros/tools/windrise/make_icons.py --check

``--check`` regenerates into a temporary tree and reports whether the committed
icons already match, without writing anything.
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
import tempfile
from io import BytesIO
from pathlib import Path

from PIL import Image, ImageDraw

PACKAGE_ROOT = Path(__file__).resolve().parents[2]
ICONS_ROOT = PACKAGE_ROOT / "resources" / "browseros" / "icons"

DEFAULT_BRAND_DIR = Path(__file__).resolve().parent / "brand-src"

BIG_MASTER = "windrise-bridge-icon-1024.png"
SMALL_MASTER = "windrise-bridge-icon-small-1024.png"
MONO_MASTER = "windrise-bridge-mark-mono-1024.png"
VECTOR_MASTER = "windrise-bridge-icon.svg"

# Brand palette.
ICON_BLUE = (0x3D, 0x97, 0xE8, 0xFF)

# Up to 32 px use the redrawn small master, matching the brand's own
# pre-rendered icon set; scaling the full art down smears the kite line and the
# screen highlight into mush.
SMALL_ART_MAX = 32

# (relative path, pixel size, source art) — mirrors the shipped file list.
PNG_TARGETS: tuple[tuple[str, int, str], ...] = (
    ("chromeos/chrome_app_icon_192.png", 192, "big"),
    ("chromeos/chrome_app_icon_32.png", 32, "small"),
    ("chromeos/crosh_app_icon_256.png", 256, "big"),
    ("chromeos/webstore_app_icon_128.png", 128, "big"),
    ("chromeos/webstore_app_icon_16.png", 16, "small"),
    ("default_100_percent/linux/product_logo_16.png", 16, "small"),
    ("default_100_percent/linux/product_logo_32.png", 32, "small"),
    ("default_100_percent/product_logo_16.png", 16, "small"),
    ("default_100_percent/product_logo_32.png", 32, "small"),
    ("default_100_percent/product_logo_name_22.png", 22, "small"),
    ("default_100_percent/product_logo_name_22_white.png", 22, "small"),
    ("default_200_percent/product_logo_16.png", 32, "small"),
    ("default_200_percent/product_logo_32.png", 64, "big"),
    ("default_200_percent/product_logo_name_22.png", 44, "big"),
    ("default_200_percent/product_logo_name_22_white.png", 44, "big"),
    ("linux/product_logo_128.png", 128, "big"),
    ("linux/product_logo_24.png", 24, "small"),
    ("linux/product_logo_256.png", 256, "big"),
    ("linux/product_logo_48.png", 48, "big"),
    ("linux/product_logo_64.png", 64, "big"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_16.png", 16, "small"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_32.png", 32, "small"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_64.png", 64, "big"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_128.png", 128, "big"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_256.png", 256, "big"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_512.png", 512, "big"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_1024.png", 1024, "big"),
    ("mac/Assets.xcassets/Icon.iconset/icon_256x256.png", 256, "big"),
    ("mac/Assets.xcassets/Icon.iconset/icon_256x256@2x.png", 512, "big"),
    ("product_logo.png", 128, "big"),
    ("product_logo_1024.png", 1024, "big"),
    ("product_logo_128.png", 128, "big"),
    ("product_logo_16.png", 16, "small"),
    ("product_logo_192.png", 192, "big"),
    ("product_logo_22.png", 22, "small"),
    ("product_logo_24.png", 24, "small"),
    ("product_logo_256.png", 256, "big"),
    ("product_logo_32.png", 32, "small"),
    ("product_logo_48.png", 48, "big"),
    ("product_logo_64.png", 64, "big"),
    ("product_logo_name_22.png", 22, "small"),
    ("product_logo_name_22_2x.png", 44, "big"),
    ("product_logo_name_22_white.png", 22, "small"),
    ("product_logo_name_22_white_2x.png", 44, "big"),
)

# Monochrome white glyphs (Linux tray, Windows tiles, mono logo slot).
MONO_TARGETS: tuple[tuple[str, int], ...] = (
    ("product_logo_22_mono.png", 22),
    ("win/tiles/Logo.png", 150),
    ("win/tiles/SmallLogo.png", 70),
)

# (relative path, sizes in the ICO) — same frames as the files being replaced.
ICO_TARGETS: tuple[tuple[str, tuple[int, ...]], ...] = (
    ("win/app_list.ico", (16, 20, 24, 32, 40, 48, 64, 128, 256)),
    ("win/chromium.ico", (16, 20, 24, 32, 40, 48, 64, 128, 256)),
    ("win/incognito.ico", (16, 20, 24, 32, 40, 48, 64, 128, 256)),
)

# Document icons: white glyph on a blue plate, so file types stay readable at
# 16 px where the full art collapses.
DOC_ICO_TARGETS: tuple[tuple[str, tuple[int, ...]], ...] = (
    ("win/chromium_doc.ico", (16, 32, 48, 256)),
    ("win/chromium_pdf.ico", (16, 32, 48, 256)),
)

SVG_TARGETS: tuple[str, ...] = (
    "product_logo.svg",
    "product_logo_animation.svg",
)

# ICNS element type -> pixel size. Mirrors an iconset so macOS gets every slot
# it looks up (1x and @2x for 16/32/128/256).
ICNS_TYPES: tuple[tuple[bytes, int], ...] = (
    (b"icp4", 16),
    (b"ic11", 32),
    (b"icp5", 32),
    (b"icp6", 64),
    (b"ic07", 128),
    (b"ic13", 256),
    (b"ic08", 256),
    (b"ic14", 512),
    (b"ic09", 512),
    (b"ic10", 1024),
)

ICNS_TARGETS: tuple[str, ...] = (
    "mac/app.icns",
    "mac/document.icns",
    "mac/AppIcon.icns",
)


def _load_masters(brand_dir: Path) -> dict[str, Image.Image]:
    masters: dict[str, Image.Image] = {}
    for key, name in (
        ("big", BIG_MASTER),
        ("small", SMALL_MASTER),
        ("mono", MONO_MASTER),
    ):
        path = brand_dir / name
        if not path.is_file():
            raise SystemExit(
                f"missing brand render: {path}\n"
                "Pass --brand-dir pointing at the directory that holds the "
                "checked-in brand-src masters."
            )
        masters[key] = Image.open(path).convert("RGBA")
    return masters


def _resize(master: Image.Image, size: int) -> Image.Image:
    if master.size == (size, size):
        return master.copy()
    return master.resize((size, size), Image.Resampling.LANCZOS)


def _art_for(size: int, masters: dict[str, Image.Image]) -> Image.Image:
    return _resize(masters["small" if size <= SMALL_ART_MAX else "big"], size)


def _white_mono(size: int, masters: dict[str, Image.Image]) -> Image.Image:
    """Recolor the flat mark to opaque white on transparent."""
    mono = _resize(masters["mono"], size)
    alpha = mono.getchannel("A")
    white = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    white.putalpha(alpha)
    return white


def _doc_icon(size: int, masters: dict[str, Image.Image]) -> Image.Image:
    """Blue plate + white mark, so document icons stay legible at 16 px and
    stay distinguishable from the app icon."""
    plate = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    margin = max(1, round(size * 0.04))
    diameter = size - 2 * margin
    circle = Image.new("RGBA", (diameter, diameter), ICON_BLUE)
    mask = Image.new("L", (diameter, diameter), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, diameter - 1, diameter - 1), fill=255)
    circle.putalpha(mask)
    plate.alpha_composite(circle, (margin, margin))

    glyph_size = max(8, round(size * 0.62))
    glyph = _white_mono(glyph_size, masters)
    offset = (size - glyph_size) // 2
    plate.alpha_composite(glyph, (offset, offset))
    return plate


def _write_png(image: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, format="PNG", optimize=True)


def _write_ico(frames: dict[int, Image.Image], sizes: tuple[int, ...], path: Path) -> None:
    """Write a PNG-in-ICO container with one pre-rendered frame per size.

    Pillow only honours ``append_images`` for ICO from 12.0 on, and silently
    downscales the largest frame on older versions — which would replace the
    redrawn small art with mush. Writing the container here keeps the output
    identical across Pillow versions.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    blobs: list[bytes] = []
    for size in sorted(sizes):
        buffer = BytesIO()
        frames[size].save(buffer, format="PNG")
        blobs.append(buffer.getvalue())

    offset = 6 + 16 * len(blobs)
    entries = []
    for size, blob in zip(sorted(sizes), blobs):
        entries.append(
            struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 0, 32,
                        len(blob), offset)
        )
        offset += len(blob)

    with path.open("wb") as handle:
        handle.write(struct.pack("<HHH", 0, 1, len(blobs)))
        handle.write(b"".join(entries))
        for blob in blobs:
            handle.write(blob)


def _image_signature(image: Image.Image) -> tuple:
    rgba = image.convert("RGBA")
    return (rgba.size, rgba.tobytes())


def _content_signature(path: Path) -> tuple:
    """Decoded content of one icon, so hosted-toolchain zlib differences
    (PNG byte streams) do not count as a mismatch."""
    if path.suffix == ".png":
        with Image.open(path) as image:
            return ("png", _image_signature(image))

    if path.suffix == ".ico":
        with Image.open(path) as image:
            frames = []
            for size in sorted(image.info["sizes"]):
                image.size = size  # pyright: ignore[reportAttributeAccessIssue]
                frames.append((size, _image_signature(image)))
            return ("ico", tuple(frames))

    if path.suffix == ".icns":
        data = path.read_bytes()
        elements = []
        offset = 8
        while offset < len(data):
            element_type = data[offset:offset + 4]
            length = struct.unpack(">I", data[offset + 4:offset + 8])[0]
            payload = data[offset + 8:offset + length]
            with Image.open(BytesIO(payload)) as image:
                elements.append((element_type, _image_signature(image)))
            offset += length
        return ("icns", tuple(elements))

    if path.suffix in (".svg", ".xpm"):
        # Text assets: a Windows checkout may hand us CRLF. Compare normalized.
        return ("text", path.read_text(encoding="utf-8").replace("\r\n", "\n"))

    return ("bytes", path.read_bytes())


def _write_icns(masters: dict[str, Image.Image], path: Path) -> None:
    """Write a PNG-based ICNS container (no Apple tooling required)."""
    path.parent.mkdir(parents=True, exist_ok=True)
    elements = []
    for element_type, size in ICNS_TYPES:
        buffer = BytesIO()
        _art_for(size, masters).save(buffer, format="PNG", optimize=True)
        payload = buffer.getvalue()
        elements.append(
            element_type + struct.pack(">I", len(payload) + 8) + payload
        )
    body = b"".join(elements)
    path.write_bytes(b"icns" + struct.pack(">I", len(body) + 8) + body)


def _write_xpm(image: Image.Image, path: Path) -> None:
    """Write an XPM3 C array (Pillow has no XPM writer).

    XPM has no alpha channel: fully transparent pixels become ``None`` and
    partially transparent pixels are composited onto white. Symbols are two
    characters wide, so the ~200 colors an antialiased 32 px icon needs fit
    (the file this replaces also used cpp=2).
    """
    rgba = image.convert("RGBA")
    width, height = rgba.size
    raw = rgba.tobytes()

    palette: dict[tuple[int, int, int] | None, int] = {}
    grid: list[int] = []
    for offset in range(0, len(raw), 4):
        red, green, blue, alpha = raw[offset:offset + 4]
        if alpha == 0:
            key = None
        else:
            if alpha != 255:
                red = (red * alpha + 255 * (255 - alpha)) // 255
                green = (green * alpha + 255 * (255 - alpha)) // 255
                blue = (blue * alpha + 255 * (255 - alpha)) // 255
            key = (red, green, blue)
        grid.append(palette.setdefault(key, len(palette)))

    if len(palette) > 92 * 92:
        raise RuntimeError(f"XPM palette too large for {path}: {len(palette)}")

    alphabet = [chr(code) for code in range(33, 127) if chr(code) not in '"\\']
    symbols = [
        alphabet[index // len(alphabet)] + alphabet[index % len(alphabet)]
        for index in range(len(palette))
    ]

    lines = [
        "/* XPM */",
        f"static char *{path.stem}[] = {{",
        "/* columns rows colors chars-per-pixel */",
        f'"{width} {height} {len(palette)} 2 ",',
    ]
    for key, index in sorted(palette.items(), key=lambda item: item[1]):
        if key is None:
            lines.append(f'"{symbols[index]} c None",')
        else:
            lines.append(
                f'"{symbols[index]} c #{key[0]:02x}{key[1]:02x}{key[2]:02x}",'
            )
    for y in range(height):
        row = "".join(symbols[grid[y * width + x]] for x in range(width))
        lines.append(f'"{row}",')
    lines.append("};")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def generate(brand_dir: Path, destination: Path) -> list[Path]:
    masters = _load_masters(brand_dir)
    written: list[Path] = []

    for relative, size, art in PNG_TARGETS:
        image = _resize(masters[art], size)
        target = destination / relative
        _write_png(image, target)
        written.append(target)

    for relative, size in MONO_TARGETS:
        target = destination / relative
        _write_png(_white_mono(size, masters), target)
        written.append(target)

    for relative, sizes in ICO_TARGETS:
        frames = {size: _art_for(size, masters) for size in sizes}
        target = destination / relative
        _write_ico(frames, sizes, target)
        written.append(target)

    for relative, sizes in DOC_ICO_TARGETS:
        frames = {size: _doc_icon(size, masters) for size in sizes}
        target = destination / relative
        _write_ico(frames, sizes, target)
        written.append(target)

    xpm = _art_for(32, masters)
    xpm_target = destination / "linux" / "product_logo_32.xpm"
    xpm_target.parent.mkdir(parents=True, exist_ok=True)
    _write_xpm(xpm, xpm_target)
    written.append(xpm_target)

    master_svg = (brand_dir / VECTOR_MASTER).read_bytes()
    for relative in SVG_TARGETS:
        target = destination / relative
        target.write_bytes(master_svg)
        written.append(target)

    for relative in ICNS_TARGETS:
        target = destination / relative
        _write_icns(masters, target)
        written.append(target)

    return written


def _relative(paths: list[Path], root: Path) -> list[str]:
    return sorted(path.relative_to(root).as_posix() for path in paths)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--brand-dir",
        type=Path,
        default=Path(os.environ.get("WINDRISE_BRAND_DIR", DEFAULT_BRAND_DIR)),
        help="Windrise brand directory holding the brand-src masters "
        "(default: %(default)s)",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=ICONS_ROOT,
        help="Icon tree to write (default: resources/browseros/icons)",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Regenerate into a temp dir and fail if the committed icons differ",
    )
    args = parser.parse_args(argv)

    brand_dir = args.brand_dir.expanduser()
    if not brand_dir.is_dir():
        print(f"brand directory not found: {brand_dir}", file=sys.stderr)
        return 2

    if not args.check:
        written = generate(brand_dir, args.out)
        print(f"wrote {len(written)} icon files under {args.out}")
        for relative in _relative(written, args.out):
            print(f"  {relative}")
        return 0

    with tempfile.TemporaryDirectory() as temporary:
        staging = Path(temporary) / "icons"
        written = generate(brand_dir, staging)
        stale = [
            relative
            for relative in _relative(written, staging)
            if not (args.out / relative).is_file()
            or _content_signature(args.out / relative)
            != _content_signature(staging / relative)
        ]
    if stale:
        print("icons differ from the brand art:", file=sys.stderr)
        for relative in stale:
            print(f"  {relative}", file=sys.stderr)
        return 1
    print("icons match the brand art")
    return 0


if __name__ == "__main__":
    sys.exit(main())
