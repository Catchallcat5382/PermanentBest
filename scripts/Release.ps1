$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

Initialize-PortableTools
Ensure-GitHubLogin
Ensure-RepositoryConnected

$latestDir = Join-Path $script:ProjectRoot 'releases\latest'
$package = Join-Path $latestDir 'PermanentBest.geode'
$versionPath = Join-Path $latestDir 'version.txt'

if (-not (Test-Path -LiteralPath $package)) {
    throw 'No latest package exists. Run SETUP_BUILD_INSTALL.bat or PUSH.bat first.'
}

if (-not (Test-Path -LiteralPath $versionPath)) {
    throw 'releases\latest\version.txt is missing.'
}

$version = (Get-Content -LiteralPath $versionPath -Raw).Trim()
$embeddedVersion = Test-GeodePackage -PackagePath $package

if ($embeddedVersion -ne $version) {
    throw "The latest package says $embeddedVersion but version.txt says $version."
}

& $script:GhExe release view $version --repo $script:Repository *> $null

if ($LASTEXITCODE -eq 0) {
    throw "GitHub release $version already exists."
}

$tagLines = @(& $script:GitExe -C $script:ProjectRoot tag --list $version 2>&1)
$tagExists = $false

foreach ($line in $tagLines) {
    if ($line.ToString().Trim() -eq $version) {
        $tagExists = $true
    }
}

if (-not $tagExists) {
    $code = Invoke-Git tag $version

    if ($code -ne 0) {
        throw "Could not create tag $version."
    }

    $code = Invoke-Git push origin $version

    if ($code -ne 0) {
        throw "Could not push tag $version."
    }
}

& $script:GhExe release create $version `
    $package `
    --repo $script:Repository `
    --title "Permanent Best $version" `
    --generate-notes

if ($LASTEXITCODE -ne 0) {
    throw 'GitHub release creation failed.'
}

Write-Host "Published GitHub release $version using releases\latest\PermanentBest.geode." -ForegroundColor Green
