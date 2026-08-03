$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

$gameRoot = Get-GameRoot
$gameExe = Join-Path $gameRoot 'GeometryDash.exe'
$flag = Join-Path $env:TEMP 'BestBarLayoutDebug.flag'

Set-Content -LiteralPath $flag -Value 'Best Bar layout debug is active.' -Encoding ASCII
try {
    Write-Host 'Layout debug enabled. Drag the top progress bar in Geometry Dash.' -ForegroundColor Yellow
    $process = Start-Process -FilePath $gameExe -WorkingDirectory $gameRoot -PassThru
    $process.WaitForExit()
}
finally {
    Remove-Item -LiteralPath $flag -Force -ErrorAction SilentlyContinue
    Write-Host 'Layout debug disabled. Saved offsets remain active.' -ForegroundColor Green
}
