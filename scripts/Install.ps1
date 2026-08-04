$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

Write-Host ''
Write-Host '============================================================' -ForegroundColor Magenta
Write-Host ' BEST BAR - INSTALL EXISTING PACKAGE' -ForegroundColor Magenta
Write-Host '============================================================' -ForegroundColor Magenta
Write-Host "Project: $script:ProjectRoot" -ForegroundColor DarkGray
Write-Host ''

$candidates = [System.Collections.Generic.List[System.IO.FileInfo]]::new()

$preferred = @(
    (Join-Path $script:ProjectRoot 'releases\latest\BestBar.geode'),
    (Join-Path $script:ProjectRoot 'dist\BestBar.geode'),
    (Join-Path $script:ProjectRoot "$($script:ModID).geode")
)

foreach ($path in $preferred) {
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $candidates.Add((Get-Item -LiteralPath $path))
    }
}

$searchRoots = @(
    (Join-Path $script:ProjectRoot 'releases\versions'),
    (Join-Path $script:ProjectRoot 'releases'),
    (Join-Path $script:ProjectRoot 'dist'),
    $script:ProjectRoot
)

foreach ($searchRoot in $searchRoots) {
    if (-not (Test-Path -LiteralPath $searchRoot -PathType Container)) {
        continue
    }

    Get-ChildItem -LiteralPath $searchRoot -Filter '*.geode' -File -Recurse -ErrorAction SilentlyContinue |
        ForEach-Object {
            if (-not ($candidates.FullName -contains $_.FullName)) {
                $candidates.Add($_)
            }
        }
}

if ($candidates.Count -eq 0) {
    throw @'
No built .geode package was found.

Run BUILD.bat first. BUILD.bat compiles, packages, and installs the mod automatically.
After a package exists, INSTALL.bat can reinstall it without rebuilding or pushing.
'@
}

$package = $candidates |
    Sort-Object @{ Expression = {
        if ($_.FullName -ieq (Join-Path $script:ProjectRoot 'releases\latest\BestBar.geode')) { 0 }
        else { 1 }
    } }, @{ Expression = 'LastWriteTimeUtc'; Descending = $true } |
    Select-Object -First 1

Write-Host "Package: $($package.FullName)" -ForegroundColor Cyan
$embeddedVersion = Test-GeodePackage -PackagePath $package.FullName
Write-Host "Version: $embeddedVersion" -ForegroundColor DarkGray
Write-Host ''

Install-GeodePackage -PackagePath $package.FullName

Write-Host ''
Write-Host 'Best Bar is now in Geometry Dash\geode\mods.' -ForegroundColor Green
Write-Host 'Start Geometry Dash normally when you are ready.' -ForegroundColor Green
