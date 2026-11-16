# Windows Install Verification Checklist

Use a Windows machine or VM with a clean user profile when verifying a fresh
install. BrowserOS installs per-user; the Apps & Features entry is under:

`HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall`

## Fresh Install

- [ ] Download the Windows installer artifact and double-click it.
- [ ] A progress window appears with the title/text `Installing BrowserOS…`.
- [ ] The installer window closes when installation completes.
- [ ] BrowserOS launches automatically after the fresh install.
- [ ] Apps & Features shows `BrowserOS`.
- [ ] Apps & Features shows publisher `BrowserOS Software Inc`.
- [ ] Apps & Features shows the BrowserOS icon.
- [ ] Apps & Features shows the expected version.
- [ ] A Start Menu shortcut exists and launches BrowserOS.
- [ ] A desktop shortcut exists and launches BrowserOS.

## Update Path

- [ ] Install an older Windrise browser version through Windrise Bridge.
- [ ] With Bridge closed, open `chrome://settings/help`: version remains visible;
      native update status, check, install and relaunch controls are absent.
- [ ] Browser menu exposes no independent browser upgrade action.
- [ ] Leave the browser running past the former one-hour update interval, then
      close and reopen it: no browser appcast request or updater installer occurs.
- [ ] Start a browser package update in Bridge using the normal signed manifest.
- [ ] Confirm Bridge alone checks, verifies and installs the package; reject an
      invalid signature or digest without replacing the installed browser.
- [ ] Relaunch the browser through the normal host flow and verify its version.
- [ ] Newly packaged browser files contain no `WinSparkle.dll`; old-version
      leftovers are not evidence that the new browser loads an updater.

## Silent Fresh Install

- [ ] Run the installer with `--silent` from Command Prompt or PowerShell.
- [ ] No installer UI appears.
- [ ] BrowserOS is installed successfully.
- [ ] BrowserOS does not auto-launch during the silent install.

## Portable Zip

- [ ] Download the portable zip artifact.
- [ ] Extract the zip.
- [ ] The extracted contents include the Windows installer executable.
