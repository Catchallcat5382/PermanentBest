$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
Normalize-ProjectMetadata
Write-Host 'Validated mod.json as UTF-8 without BOM.' -ForegroundColor Green
