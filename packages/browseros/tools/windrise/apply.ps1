#requires -Version 5.1
<#
.SYNOPSIS
  Re-apply only the Chromium patches changed in a commit range.

.DESCRIPTION
  The build machine already has an older patch series applied and a configured
  out/ directory. Never run clean, download_resources or series_patches there:
  those steps assume a pristine checkout. This script resets only the files
  touched by the changed patches, applies those patches with `git apply
  --recount`, then re-runs the resource/string/module steps the build pipeline
  normally runs during prep.

  The script deliberately prints (but does not execute) the autoninja command.

.EXAMPLE
  .\apply.ps1 -From b8de97a6e -To HEAD

.EXAMPLE
  .\apply.ps1 -From 328c48a00 -To HEAD -ChromiumSrc C:\src\chromium\src
#>
[CmdletBinding()]
param(
    [string]$From = "HEAD~1",
    [string]$To = "HEAD",
    [string]$BrowserosRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path,
    [string]$ChromiumSrc = "C:\src\chromium\src",
    [ValidateSet("browseros", "browserclaw")]
    [string]$Product = "browseros",
    [ValidateSet("debug", "release")]
    [string]$BuildType = "release",
    [ValidateSet("x64", "arm64")]
    [string]$Arch = "x64",
    [string]$OutDir = "",
    [string]$WindriseExtensionDir = $env:WINDRISE_EXTENSION_DIR
)

$ErrorActionPreference = "Stop"

function Get-GitOutput {
    param([string]$Repo, [string[]]$GitArgs)
    $output = & git -C $Repo @GitArgs
    if ($LASTEXITCODE -ne 0) {
        throw "git -C '$Repo' $($GitArgs -join ' ') failed with exit code $LASTEXITCODE"
    }
    return @($output)
}

function Skip-PatchFile {
    param([string]$RelativePath)
    $name = Split-Path -Leaf $RelativePath
    if ($name.StartsWith(".")) { return $true }
    foreach ($suffix in @(".deleted", ".binary", ".rename")) {
        if ($name.EndsWith($suffix)) { return $true }
    }
    return $false
}

function Get-PatchTarget {
    param([string]$PatchText)
    $match = [regex]::Match($PatchText, "(?m)^diff --git a/.+? b/(?<path>.+)$")
    if ($match.Success) { return $match.Groups["path"].Value.Trim() }
    $match = [regex]::Match($PatchText, "(?m)^\+\+\+ b/(?<path>.+)$")
    if ($match.Success) { return $match.Groups["path"].Value.Trim() }
    return $null
}

function Test-NewFilePatch {
    param([string]$PatchText)
    return [regex]::IsMatch($PatchText, "(?m)^new file mode ") -or
        [regex]::IsMatch($PatchText, "(?m)^--- /dev/null$")
}

function Invoke-ChromiumCheckout {
    param([string]$Path)
    $result = & git -C $ChromiumSrc checkout -- $Path 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Could not restore upstream '$Path' in '$ChromiumSrc': $result"
    }
}

function Invoke-ChromiumRemove {
    param([string]$Path)
    $absolute = Join-Path $ChromiumSrc $Path
    if (Test-Path -LiteralPath $absolute) {
        Remove-Item -LiteralPath $absolute -Force
    }
}

function Invoke-BuildModule {
    param([string]$Modules)
    Write-Host "`n==> uv run browseros build --modules $Modules" -ForegroundColor Cyan
    Push-Location $BrowserosRoot
    try {
        & uv run browseros build `
            --modules $Modules `
            --product $Product `
            --build-type $BuildType `
            --arch $Arch `
            --chromium-src $ChromiumSrc
        if ($LASTEXITCODE -ne 0) {
            throw "browseros build --modules $Modules failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

if (-not (Test-Path -LiteralPath $ChromiumSrc)) {
    throw "Chromium source not found: $ChromiumSrc"
}
if (-not (Test-Path -LiteralPath (Join-Path $BrowserosRoot "chromium_patches"))) {
    throw "chromium_patches not found under $BrowserosRoot"
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required"
}
if (-not (Get-Command uv -ErrorAction SilentlyContinue)) {
    throw "uv is required"
}

$changed = @(
    Get-GitOutput -Repo $BrowserosRoot -GitArgs @(
        "diff", "--name-status", "--relative", $From, $To, "--", "chromium_patches"
    )
)

$patchRows = @()
foreach ($line in $changed) {
    if ([string]::IsNullOrWhiteSpace($line)) { continue }
    $parts = $line -split "`t", 2
    if ($parts.Count -ne 2) { continue }
    $status = $parts[0]
    $relative = $parts[1].Trim()
    if ($relative -notlike "chromium_patches*") { continue }
    if (Skip-PatchFile $relative) { continue }
    $patchRows += [pscustomobject]@{ Status = $status; Relative = $relative }
}

if ($patchRows.Count -eq 0) {
    Write-Host "No chromium patch files changed between '$From' and '$To'." -ForegroundColor Yellow
    exit 0
}

Write-Host "Changed patch files ($From..$To):" -ForegroundColor Cyan
$patchRows | ForEach-Object { Write-Host ("  {0}  {1}" -f $_.Status, $_.Relative) }

$applied = 0
foreach ($row in $patchRows) {
    $patchPath = Join-Path $BrowserosRoot $row.Relative
    $target = $row.Relative

    if ($row.Status -eq "D" -or -not (Test-Path -LiteralPath $patchPath)) {
        # The patch itself was removed. Put the Chromium file back to upstream
        # (or remove it if it only ever existed as a BrowserOS-created file).
        $targetPath = $target -replace "^chromium_patches[\\/]", ""
        $existsUpstream = $false
        & git -C $ChromiumSrc cat-file -e "HEAD:$targetPath" 2>$null
        if ($LASTEXITCODE -eq 0) { $existsUpstream = $true }
        if ($existsUpstream) {
            Invoke-ChromiumCheckout $targetPath
            Write-Host "  restored upstream: $targetPath" -ForegroundColor Yellow
        }
        else {
            Invoke-ChromiumRemove $targetPath
            Write-Host "  removed BrowserOS-only file: $targetPath" -ForegroundColor Yellow
        }
        continue
    }

    $text = [IO.File]::ReadAllText($patchPath)
    $targetPath = Get-PatchTarget $text
    if (-not $targetPath) {
        throw "Could not parse a target path from $patchPath"
    }

    Write-Host "`n==> $targetPath" -ForegroundColor Green
    if (Test-NewFilePatch $text) {
        Invoke-ChromiumRemove $targetPath
    }
    else {
        Invoke-ChromiumCheckout $targetPath
    }

    & git -C $ChromiumSrc apply --recount --whitespace=nowarn -p1 $patchPath
    if ($LASTEXITCODE -ne 0) {
        throw "git apply --recount failed for $patchPath"
    }
    $applied++
}

Write-Host "`nApplied $applied changed patch file(s)." -ForegroundColor Green

Invoke-BuildModule "chromium_replace"

Write-Host "`n==> reset branded string files before string_replaces" -ForegroundColor Cyan
Invoke-ChromiumCheckout "chrome/app/chromium_strings.grd"
Invoke-ChromiumCheckout "chrome/app/settings_chromium_strings.grdp"
Invoke-BuildModule "string_replaces"

Invoke-BuildModule "resources,bundled_extensions"

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = "out/Default_${Product}_${Arch}"
}

if (-not [string]::IsNullOrWhiteSpace($WindriseExtensionDir) -and (Test-Path -LiteralPath $WindriseExtensionDir)) {
    $stageScript = Join-Path $PSScriptRoot "stage_windrise_extension.py"
    $pythonCmd = Get-Command python -ErrorAction SilentlyContinue
    if (-not $pythonCmd) { $pythonCmd = Get-Command python3 -ErrorAction SilentlyContinue }
    if ($pythonCmd -and (Test-Path -LiteralPath $stageScript)) {
        Write-Host "`n==> Staging signed windrise-extension into $OutDir/windrise_extensions/windrise-extension" -ForegroundColor Cyan
        & $pythonCmd.Source $stageScript `
            --source $WindriseExtensionDir `
            --dest (Join-Path $ChromiumSrc "$OutDir\windrise_extensions\windrise-extension")
        if ($LASTEXITCODE -ne 0) {
            throw "stage_windrise_extension.py failed with exit code $LASTEXITCODE"
        }
    }
}
$ninja = if ($env:OS -eq "Windows_NT") { "autoninja.bat" } else { "autoninja" }
Write-Host "`nPatches and prep modules are up to date." -ForegroundColor Green
Write-Host "Do NOT run clean, download_resources or series_patches on this checkout." -ForegroundColor Yellow
Write-Host "`nNext command (not executed):" -ForegroundColor Cyan
Write-Host "  $ninja -C $OutDir chrome chromedriver"
