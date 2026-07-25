$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

$gameRoot = Get-GameRoot
$gameExe = Join-Path $gameRoot 'GeometryDash.exe'

$process = Start-Process -FilePath $gameExe -WorkingDirectory $gameRoot -PassThru
Start-Sleep -Seconds 5

if ($process.HasExited) {
    throw "Geometry Dash closed immediately with exit code $($process.ExitCode)."
}
