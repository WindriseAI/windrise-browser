#!/usr/bin/env python3
"""Generate the Windrise Browser icon set from the B6 brand finals.

Replaces every raster icon under ``resources/browseros/icons`` with a Windrise
version at the same path and the same pixel size, so the Chromium overlay copy
step (``bos_build/config/copy_resources.yaml``) needs no changes.

It also regenerates the in-repo copies the Chromium build cannot reach: the
agent extension and onboarding WebUI favicons, the extension's product logo
SVG, and ``resources/logo.png``.

Source art lives next to this script in ``brand-src/`` — the checked-in copy of
the B6 brand export, so the build never reads a directory outside the repo:

* ``windrise-browser-icon-<size>.png``      — app icon, one hand-tuned render
  per size: 16/24/32/48 are separately bolded artwork, 64..1024 the standard
  artwork. The brand ships no 20/22/40/44/96/150/192 render, so those are
  resampled from the closest render (from the bold family below 48 px, from the
  next larger render above it) instead of from the 1024 master.
* ``windrise-browser-mark-white-<size>.png`` — bare glyph for dark surfaces
  (16/24/32/48/256); fills the mono glyph slots, the Windows tiles and the
  ``*_white`` logo slots. The export's 1024 px white glyph is not checked in
  because no consumer slot asks for it, and neither is the ink variant: no slot
  needs a light-background glyph yet.
* ``windrise-browser-icon-256.svg``          — vector master, copied as-is for
  the ``product_logo*.svg`` slots. The per-size 16/24/32/48 SVGs are bolded for
  their own pixel grid and must not be scaled up, so only the 256 vector is
  checked in.

Windows ``.ico`` and macOS ``.icns`` are assembled here from those same finals:
``win/chromium.ico``/``app_list.ico``/``incognito.ico`` carry the 定稿 ICO frame
set (16/24/32/48/64/128/256) and ``mac/*.icns`` the 定稿 ICNS element set
(ic07/ic08/ic09/ic10/ic11/ic12/ic13/ic14). Every frame is a pre-rendered 定稿
size, so the assembled containers are pixel-identical to ``windrise-browser.ico``
and ``windrise-browser.icns`` while staying inside the PNG/SVG-only brand-src
contract. The brand masters are already rasterized, so this script needs no SVG
renderer and stays reproducible with Pillow only.

macOS ``Assets.car`` is an Apple asset catalog that only ``actool`` (macOS +
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
import math
import os
import struct
import sys
import tempfile
from io import BytesIO
from pathlib import Path

from PIL import Image, ImageDraw

PACKAGE_ROOT = Path(__file__).resolve().parents[2]
REPO_ROOT = PACKAGE_ROOT.parents[1]
ICONS_ROOT = PACKAGE_ROOT / "resources" / "browseros" / "icons"
APP_ICONS_ROOT = REPO_ROOT / "packages/browseros-agent/apps/app/public/icon"
ONBOARD_ICONS_ROOT = (
    REPO_ROOT / "packages/browseros-agent/apps/app-onboard/public/icon"
)
APP_LOGO_SVG = (
    REPO_ROOT / "packages/browseros-agent/apps/app/assets/product_logo.svg"
)
REPO_LOGO_PNG = PACKAGE_ROOT / "resources" / "logo.png"

DEFAULT_BRAND_DIR = Path(__file__).resolve().parent / "brand-src"

# Brand palette, still used for the document-icon plate (no B6 final exists
# for document types yet).
ICON_BLUE = (0x3D, 0x97, 0xE8, 0xFF)

VECTOR_MASTER = "windrise-browser-icon-256.svg"

# 定稿 B6 draws 16/24/32/48 as separately bolded artwork; scaling the standard
# artwork down to those sizes smears the light trail.
BOLD_MAX = 48

FAMILIES: dict[str, tuple[str, tuple[int, ...]]] = {
    "icon": ("windrise-browser-icon-{size}.png",
             (16, 24, 32, 48, 64, 128, 256, 512, 1024)),
    "mark": ("windrise-browser-mark-white-{size}.png",
             (16, 24, 32, 48, 256)),
}

# (relative path, pixel size, brand family) — mirrors the shipped file list.
PNG_TARGETS: tuple[tuple[str, int, str], ...] = (
    ("chromeos/chrome_app_icon_192.png", 192, "icon"),
    ("chromeos/chrome_app_icon_32.png", 32, "icon"),
    ("chromeos/crosh_app_icon_256.png", 256, "icon"),
    ("chromeos/webstore_app_icon_128.png", 128, "icon"),
    ("chromeos/webstore_app_icon_16.png", 16, "icon"),
    ("default_100_percent/linux/product_logo_16.png", 16, "icon"),
    ("default_100_percent/linux/product_logo_32.png", 32, "icon"),
    ("default_100_percent/product_logo_16.png", 16, "icon"),
    ("default_100_percent/product_logo_32.png", 32, "icon"),
    ("default_100_percent/product_logo_name_22.png", 22, "icon"),
    ("default_100_percent/product_logo_name_22_white.png", 22, "mark"),
    ("default_200_percent/product_logo_16.png", 32, "icon"),
    ("default_200_percent/product_logo_32.png", 64, "icon"),
    ("default_200_percent/product_logo_name_22.png", 44, "icon"),
    ("default_200_percent/product_logo_name_22_white.png", 44, "mark"),
    ("linux/product_logo_128.png", 128, "icon"),
    ("linux/product_logo_24.png", 24, "icon"),
    ("linux/product_logo_256.png", 256, "icon"),
    ("linux/product_logo_48.png", 48, "icon"),
    ("linux/product_logo_64.png", 64, "icon"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_16.png", 16, "icon"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_32.png", 32, "icon"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_64.png", 64, "icon"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_128.png", 128, "icon"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_256.png", 256, "icon"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_512.png", 512, "icon"),
    ("mac/Assets.xcassets/AppIcon.appiconset/appicon_1024.png", 1024, "icon"),
    ("mac/Assets.xcassets/Icon.iconset/icon_256x256.png", 256, "icon"),
    ("mac/Assets.xcassets/Icon.iconset/icon_256x256@2x.png", 512, "icon"),
    ("product_logo.png", 128, "icon"),
    ("product_logo_1024.png", 1024, "icon"),
    ("product_logo_128.png", 128, "icon"),
    ("product_logo_16.png", 16, "icon"),
    ("product_logo_192.png", 192, "icon"),
    ("product_logo_22.png", 22, "icon"),
    ("product_logo_24.png", 24, "icon"),
    ("product_logo_256.png", 256, "icon"),
    ("product_logo_32.png", 32, "icon"),
    ("product_logo_48.png", 48, "icon"),
    ("product_logo_64.png", 64, "icon"),
    ("product_logo_name_22.png", 22, "icon"),
    ("product_logo_name_22_2x.png", 44, "icon"),
    ("product_logo_name_22_white.png", 22, "mark"),
    ("product_logo_name_22_white_2x.png", 44, "mark"),
)

# Monochrome white glyphs (Linux tray, Windows tiles, mono logo slot).
MARK_TARGETS: tuple[tuple[str, int], ...] = (
    ("product_logo_22_mono.png", 22),
    ("win/tiles/Logo.png", 150),
    ("win/tiles/SmallLogo.png", 70),
)

# The 定稿 ICO frame set, one pre-rendered frame per size.
APP_ICO_SIZES: tuple[int, ...] = (16, 24, 32, 48, 64, 128, 256)
ICO_TARGETS: tuple[tuple[str, tuple[int, ...]], ...] = (
    ("win/app_list.ico", APP_ICO_SIZES),
    ("win/chromium.ico", APP_ICO_SIZES),
    ("win/incognito.ico", APP_ICO_SIZES),
)

# Document icons: white glyph on a blue plate, so file types stay readable at
# 16 px and stay distinguishable from the app icon.
DOC_ICO_TARGETS: tuple[tuple[str, tuple[int, ...]], ...] = (
    ("win/chromium_doc.ico", (16, 32, 48, 256)),
    ("win/chromium_pdf.ico", (16, 32, 48, 256)),
)

SVG_TARGETS: tuple[str, ...] = (
    "product_logo.svg",
    "product_logo_animation.svg",
)

# ICNS element type -> pixel size, mirroring the 定稿 windrise-browser.icns
# element set (the brand export carries no 16 px element).
ICNS_TYPES: tuple[tuple[bytes, int], ...] = (
    (b"ic07", 128),
    (b"ic08", 256),
    (b"ic09", 512),
    (b"ic10", 1024),
    (b"ic11", 32),
    (b"ic12", 64),
    (b"ic13", 256),
    (b"ic14", 512),
)

ICNS_TARGETS: tuple[str, ...] = (
    "mac/app.icns",
    "mac/document.icns",
    "mac/AppIcon.icns",
)

# The agent extension and its onboarding WebUI ship their own icon copies.
# Generate those from the same masters so they cannot drift from the browser.
UI_ICON_SIZES: tuple[int, ...] = (16, 32, 48, 96, 128)
UI_PNG_TARGETS: tuple[tuple[str, int], ...] = tuple(
    (root.relative_to(REPO_ROOT).as_posix() + f"/{size}.png", size)
    for root in (APP_ICONS_ROOT, ONBOARD_ICONS_ROOT)
    for size in UI_ICON_SIZES
)
APP_LOGO_SVG_RELATIVE = APP_LOGO_SVG.relative_to(REPO_ROOT).as_posix()
REPO_LOGO_RELATIVE = REPO_LOGO_PNG.relative_to(REPO_ROOT).as_posix()
REPO_LOGO_SIZE = 512


class BrandArt:
    """B6 brand finals, keyed by brand family and pixel size."""

    def __init__(self, brand_dir: Path) -> None:
        self.brand_dir = brand_dir
        self._finals: dict[tuple[str, int], tuple[bytes, Image.Image]] = {}

    def final(self, family: str, size: int) -> tuple[bytes, Image.Image]:
        """Raw bytes and decoded pixels of one checked-in 定稿 render."""
        cached = self._finals.get((family, size))
        if cached is not None:
            return cached
        pattern, sizes = FAMILIES[family]
        if size not in sizes:
            raise SystemExit(f"the B6 export has no {family} render at {size} px")
        path = self.brand_dir / pattern.format(size=size)
        if not path.is_file():
            raise SystemExit(
                f"missing brand render: {path}\n"
                "Pass --brand-dir pointing at the directory that holds the "
                "checked-in brand-src masters."
            )
        raw = path.read_bytes()
        with Image.open(path) as opened:
            image = opened.convert("RGBA")
        if image.size != (size, size):
            raise SystemExit(
                f"{path.name} is {image.size[0]}x{image.size[1]}, "
                f"expected {size}x{size}"
            )
        self._finals[(family, size)] = (raw, image)
        return raw, image

    def render(
        self, family: str, size: int
    ) -> tuple[Image.Image, bytes | None]:
        """Image for one output size, plus PNG bytes when it is a 定稿 render."""
        source = self._source_size(size, FAMILIES[family][1])
        raw, image = self.final(family, source)
        if source == size:
            return image, raw
        return _resize(image, size), None

    def blob(self, family: str, size: int) -> bytes:
        image, raw = self.render(family, size)
        return raw if raw is not None else _png_blob(image)

    @staticmethod
    def _source_size(size: int, sizes: tuple[int, ...]) -> int:
        if size in sizes:
            return size
        bold = tuple(candidate for candidate in sizes if candidate <= BOLD_MAX)
        if size <= BOLD_MAX:
            return min(bold, key=lambda candidate: abs(math.log(size / candidate)))
        larger = [candidate for candidate in sizes if candidate > size]
        return min(larger) if larger else max(sizes)


def _resize(master: Image.Image, size: int) -> Image.Image:
    return master.resize((size, size), Image.Resampling.LANCZOS)


def _png_blob(image: Image.Image) -> bytes:
    buffer = BytesIO()
    image.save(buffer, format="PNG", optimize=True)
    return buffer.getvalue()


def _doc_icon(size: int, art: BrandArt) -> Image.Image:
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
    glyph = art.render("mark", glyph_size)[0]
    offset = (size - glyph_size) // 2
    plate.alpha_composite(glyph, (offset, offset))
    return plate


def _write_png(image: Image.Image, path: Path, raw: bytes | None = None) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if raw is not None:
        path.write_bytes(raw)
    else:
        image.save(path, format="PNG", optimize=True)


def _write_ico(blobs: dict[int, bytes], path: Path) -> None:
    """Write a PNG-in-ICO container with one pre-rendered frame per size.

    Pillow only honours ``append_images`` for ICO from 12.0 on, and silently
    downscales the largest frame on older versions — which would replace the
    per-size 定稿 frames with mush. Writing the container here keeps the output
    identical across Pillow versions.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    sizes = sorted(blobs)
    entries = []
    offset = 6 + 16 * len(sizes)
    for size in sizes:
        blob = blobs[size]
        entries.append(
            struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 0, 32,
                        len(blob), offset)
        )
        offset += len(blob)

    with path.open("wb") as handle:
        handle.write(struct.pack("<HHH", 0, 1, len(sizes)))
        handle.write(b"".join(entries))
        for size in sizes:
            handle.write(blobs[size])


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
        return ("icns", _icns_elements(path.read_bytes()))

    if path.suffix in (".svg", ".xpm"):
        # Text assets: a Windows checkout may hand us CRLF. Compare normalized.
        return ("text", path.read_text(encoding="utf-8").replace("\r\n", "\n"))

    return ("bytes", path.read_bytes())


def _icns_elements(data: bytes) -> tuple:
    """Decoded image elements of an ICNS container, ordered by element type.

    ``iconutil`` output (the brand export) carries a leading ``TOC `` element
    that is an index, not an image, so it is skipped: a container assembled
    here compares equal to the exported one whenever the artwork matches.
    """
    elements: list[tuple[bytes, tuple]] = []
    offset = 8
    while offset + 8 <= len(data):
        element_type = data[offset:offset + 4]
        length = struct.unpack(">I", data[offset + 4:offset + 8])[0]
        payload = data[offset + 8:offset + length]
        if length >= 8:
            try:
                with Image.open(BytesIO(payload)) as image:
                    elements.append((element_type, _image_signature(image)))
            except OSError:
                pass  # Index or unsupported element: not artwork.
        offset += length
    return tuple(sorted(elements, key=lambda element: element[0]))


def _write_icns(art: BrandArt, path: Path) -> None:
    """Write a PNG-based ICNS container (no Apple tooling required)."""
    path.parent.mkdir(parents=True, exist_ok=True)
    elements = []
    for element_type, size in ICNS_TYPES:
        payload = art.blob("icon", size)
        elements.append(element_type + struct.pack(">I", len(payload) + 8) + payload)
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
    art = BrandArt(brand_dir)
    written: list[Path] = []

    for relative, size, family in PNG_TARGETS:
        image, raw = art.render(family, size)
        target = destination / relative
        _write_png(image, target, raw)
        written.append(target)

    for relative, size in MARK_TARGETS:
        image, raw = art.render("mark", size)
        target = destination / relative
        _write_png(image, target, raw)
        written.append(target)

    for relative, sizes in ICO_TARGETS:
        target = destination / relative
        _write_ico({size: art.blob("icon", size) for size in sizes}, target)
        written.append(target)

    for relative, sizes in DOC_ICO_TARGETS:
        target = destination / relative
        # 2026-11-12：doc/pdf 图标改用 brand-src 里的成品 ico（brand-ext 设计定稿），
        # 不再现场合成——与应用图标 ICO 帧直用的做法一致。
        source_ico = brand_dir / f"{target.stem}.ico"
        target.write_bytes(source_ico.read_bytes())
        written.append(target)

    xpm_target = destination / "linux" / "product_logo_32.xpm"
    xpm_target.parent.mkdir(parents=True, exist_ok=True)
    _write_xpm(art.render("icon", 32)[0], xpm_target)
    written.append(xpm_target)

    master_svg = (brand_dir / VECTOR_MASTER).read_bytes()
    for relative in SVG_TARGETS:
        target = destination / relative
        target.write_bytes(master_svg)
        written.append(target)

    for relative in ICNS_TARGETS:
        target = destination / relative
        _write_icns(art, target)
        written.append(target)

    return written


def generate_ui(brand_dir: Path, destination: Path) -> list[Path]:
    """Generate the extension/onboarding icons and the repo logo.

    ``destination`` is the repository root (or a staging tree with the same
    relative layout), so callers can dry-run the whole set.
    """
    art = BrandArt(brand_dir)
    written: list[Path] = []

    for relative, size in UI_PNG_TARGETS:
        image, raw = art.render("icon", size)
        _write_png(image, destination / relative, raw)
        written.append(destination / relative)

    logo_svg = destination / APP_LOGO_SVG_RELATIVE
    logo_svg.parent.mkdir(parents=True, exist_ok=True)
    logo_svg.write_bytes((brand_dir / VECTOR_MASTER).read_bytes())
    written.append(logo_svg)

    image, raw = art.render("icon", REPO_LOGO_SIZE)
    repo_logo = destination / REPO_LOGO_RELATIVE
    _write_png(image, repo_logo, raw)
    written.append(repo_logo)

    return written


def _stale_paths(
    committed_root: Path, staging_root: Path, written: list[Path]
) -> list[str]:
    return [
        relative
        for relative in _relative(written, staging_root)
        if not (committed_root / relative).is_file()
        or _content_signature(committed_root / relative)
        != _content_signature(staging_root / relative)
    ]


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
        written_ui = generate_ui(brand_dir, REPO_ROOT)
        print(f"wrote {len(written_ui)} app icons under {REPO_ROOT}")
        for relative in _relative(written_ui, REPO_ROOT):
            print(f"  {relative}")
        return 0

    with tempfile.TemporaryDirectory() as temporary:
        staging = Path(temporary) / "icons"
        written = generate(brand_dir, staging)
        stale = _stale_paths(args.out, staging, written)

        ui_staging = Path(temporary) / "repo"
        written_ui = generate_ui(brand_dir, ui_staging)
        stale += _stale_paths(REPO_ROOT, ui_staging, written_ui)
    if stale:
        print("icons differ from the brand art:", file=sys.stderr)
        for relative in stale:
            print(f"  {relative}", file=sys.stderr)
        return 1
    print("icons match the brand art")
    return 0


if __name__ == "__main__":
    sys.exit(main())
