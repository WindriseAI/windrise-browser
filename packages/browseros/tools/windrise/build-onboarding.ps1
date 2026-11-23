#requires -Version 5.1
<#
.SYNOPSIS
  Build the onboarding WebUI (chrome://browseros-onboarding/) from source and
  stage it where copy_resources.yaml expects it.

.DESCRIPTION
  Upstream downloads app-onboard/prod-resources/latest/browseros-app-onboard-
  resources.zip from R2 (download_resources.yaml). Windrise has no access to
  that bucket, so without this step the Chromium tree only carries placeholder
  files and the onboarding page renders an empty "Windrise" heading.

  This script runs `vite build --mode chromium` in apps/app-onboard, checks the
  result with scripts/verify-chromium-build.ts (the same contract as BUILD.gn's
  _browseros_onboarding_resource_files: index.html, app.css, app.js, icon/*.png)
  and copies it to resources/binaries/browseros_onboarding/resources, which the
  "Browser Onboarding Resources" copy operation then puts into
  chrome/browser/browseros/onboarding/resources.

  With -ChromiumSrc it also copies straight into the Chromium tree, for build
  machines that do not re-run the resources module.

  `tsc --noEmit` is skipped on purpose when typescript is not installed; the
  Vite build itself does not depend on it.

.EXAMPLE
  .\build-onboarding.ps1 -Bun C:\tools\bun.exe -ChromiumSrc C:\src\chromium\src
#>
[CmdletBinding()]
param(
    [string]$BrowserosRoot = "",
    [string]$Bun = "bun",
    [string]$ChromiumSrc = ""
)

$ErrorActionPreference = "Stop"

if (-not $BrowserosRoot) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $BrowserosRoot = (Resolve-Path (Join-Path $scriptDir "..\..")).Path
}

$agentRoot = Join-Path (Split-Path -Parent $BrowserosRoot) "browseros-agent"
$appDir = Join-Path $agentRoot "apps\app-onboard"
$distDir = Join-Path $appDir "dist\chromium"
$stageDir = Join-Path $BrowserosRoot "resources\binaries\browseros_onboarding\resources"
$toolsDir = Join-Path $BrowserosRoot "tools\windrise"
$brandSrcDir = Join-Path $toolsDir "brand-src"
$publicIconDir = Join-Path $appDir "public\icon"

# Ensure B6 brand icons (especially icon/128.png rendered by VisualRail) are
# materialized under apps/app-onboard/public/icon before Vite builds.
New-Item -ItemType Directory -Force -Path $publicIconDir | Out-Null
$makeIcons = Join-Path $toolsDir "make_icons.py"
$pythonCmd = Get-Command python -ErrorAction SilentlyContinue
if (-not $pythonCmd) { $pythonCmd = Get-Command python3 -ErrorAction SilentlyContinue }
$iconsSynced = $false
if ($pythonCmd -and (Test-Path -LiteralPath $makeIcons)) {
    & $pythonCmd.Source $makeIcons --check *> $null
    if ($LASTEXITCODE -eq 0) {
        $iconsSynced = $true
    } else {
        & $pythonCmd.Source $makeIcons
        if ($LASTEXITCODE -eq 0) { $iconsSynced = $true }
    }
}
if (-not $iconsSynced) {
    foreach ($size in @(16, 32, 48, 128)) {
        $srcIcon = Join-Path $brandSrcDir "windrise-browser-icon-$size.png"
        if (Test-Path -LiteralPath $srcIcon) {
            Copy-Item -Force $srcIcon (Join-Path $publicIconDir "$size.png")
        }
    }
}
$icon128 = Join-Path $publicIconDir "128.png"
if (-not (Test-Path -LiteralPath $icon128) -or (Get-Item -LiteralPath $icon128).Length -le 0) {
    throw "Missing B6 onboarding icon: $icon128"
}

if (-not (Test-Path (Join-Path $agentRoot "node_modules"))) {
    Push-Location $agentRoot
    try {
        & $Bun install --filter "@browseros/app-onboard" --ignore-scripts
        if ($LASTEXITCODE -ne 0) { throw "bun install failed ($LASTEXITCODE)" }
    } finally { Pop-Location }
}

Push-Location $appDir
try {
    & $Bun x --bun vite build --mode chromium
    if ($LASTEXITCODE -ne 0) { throw "vite build failed ($LASTEXITCODE)" }
    & $Bun scripts/verify-chromium-build.ts
    if ($LASTEXITCODE -ne 0) { throw "verify-chromium-build failed ($LASTEXITCODE)" }
} finally { Pop-Location }

function Copy-Resources([string]$Destination) {
    if (Test-Path $Destination) { Remove-Item -Recurse -Force $Destination }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Copy-Item -Recurse -Force (Join-Path $distDir "*") $Destination
    $stagedIcon128 = Join-Path $Destination "icon\128.png"
    if (-not (Test-Path -LiteralPath $stagedIcon128) -or (Get-Item -LiteralPath $stagedIcon128).Length -le 0) {
        throw "Staged onboarding resources missing icon/128.png under $Destination"
    }
    Write-Host "Staged onboarding resources -> $Destination (verified icon/128.png)"
}

Copy-Resources $stageDir
if ($ChromiumSrc) {
    Copy-Resources (Join-Path $ChromiumSrc "chrome\browser\browseros\onboarding\resources")
}
