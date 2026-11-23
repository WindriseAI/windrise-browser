#requires -Version 5.1
<#
.SYNOPSIS
  Stage a signed windrise-extension directory into a Chromium build output
  directory so standalone browser launches load Windrise NTP and settings
  without waiting for the desktop app.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,
    [Parameter(Mandatory = $true)]
    [string]$OutDir,
    [switch]$CheckOnly
)

$ErrorActionPreference = "Stop"

$scriptPath = Join-Path $PSScriptRoot "stage_windrise_extension.py"
$pythonCmd = Get-Command python -ErrorAction SilentlyContinue
if (-not $pythonCmd) { $pythonCmd = Get-Command python3 -ErrorAction SilentlyContinue }
if (-not $pythonCmd) { throw "python is required to stage windrise-extension" }

$destDir = Join-Path $OutDir "windrise_extensions\windrise-extension"
$argsList = @($scriptPath, "--source", $SourceDir)
if ($CheckOnly) {
    $argsList += "--check"
} else {
    $argsList += @("--dest", $destDir)
}

& $pythonCmd.Source @argsList
if ($LASTEXITCODE -ne 0) {
    throw "stage_windrise_extension.py failed with exit code $LASTEXITCODE"
}
