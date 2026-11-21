# Contributing to Windrise Browser

Thank you for your interest in contributing to Windrise Browser.

## Prerequisites

- Chromium source tree checked out at the pinned baseline (`158.0.8127.41` / `3479de84ee`)
- Python 3.12+
- Platform C++ toolchain (Visual Studio Build Tools on Windows, Xcode Command Line Tools on macOS, or `build-essential` on Linux)

## Patch Workflow

All Chromium modifications live as unified diff patches under `packages/browseros/chromium_patches/`.
After editing any patch file, always recount hunks and verify with `git apply`:

```bash
python packages/browseros/tools/windrise/recount_patches.py
python packages/browseros/tools/windrise/recount_patches.py --check --git
```

## License

Contributions are licensed under the GNU Affero General Public License v3.0 (see [LICENSE](LICENSE)).
