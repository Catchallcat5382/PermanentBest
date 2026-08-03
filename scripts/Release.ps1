$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

Initialize-PortableTools
Ensure-GitHubLogin
Ensure-RepositoryConnected
Normalize-ProjectMetadata

$currentVersion = ConvertTo-NormalizedModVersion -Version (Get-ModVersion)
$enteredVersion = Read-Host "Version to release [$currentVersion]"
$version = if ([string]::IsNullOrWhiteSpace($enteredVersion)) {
    $currentVersion
}
else {
    ConvertTo-NormalizedModVersion -Version $enteredVersion
}

$releaseRoot = Join-Path $script:ProjectRoot 'releases'
$versionDir = Join-Path $releaseRoot "versions\$version"
$versionedPackage = Join-Path $versionDir "BestBar-$version.geode"
$latestPackage = Join-Path $releaseRoot 'latest\BestBar.geode'
$package = $null

if (Test-Path -LiteralPath $versionedPackage) {
    $package = $versionedPackage
}
elseif (Test-Path -LiteralPath $latestPackage) {
    $latestEmbedded = Test-GeodePackage -PackagePath $latestPackage
    if ($latestEmbedded -eq $version) {
        New-Item -ItemType Directory -Path $versionDir -Force | Out-Null
        Copy-Item -LiteralPath $latestPackage -Destination $versionedPackage -Force
        $package = $versionedPackage
    }
}

if ([string]::IsNullOrWhiteSpace($package)) {
    throw "No built package for $version was found. Run PUSH.bat with that version first, or build that version so releases\latest contains it."
}

$embeddedVersion = Test-GeodePackage -PackagePath $package
if ($embeddedVersion -ne $version) {
    throw "The selected package contains $embeddedVersion, not $version."
}

& $script:GhExe release view $version --repo $script:Repository *> $null
$releaseExists = ($LASTEXITCODE -eq 0)

if ($releaseExists) {
    Write-Host "Release $version already exists. Replacing its .geode asset..." -ForegroundColor Yellow
    & $script:GhExe release upload $version $package --repo $script:Repository --clobber
    if ($LASTEXITCODE -ne 0) {
        throw "Could not update GitHub release $version."
    }
    Write-Host "Updated GitHub release $version from $package." -ForegroundColor Green
    exit 0
}

$null = Invoke-GitQuiet fetch origin --tags

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
}

$code = Invoke-Git push origin $version
if ($code -ne 0) {
    throw "Could not push tag $version."
}

$releaseArgs = @(
    'release', 'create', $version, $package,
    '--repo', $script:Repository,
    '--title', "Best Bar $version",
    '--target', 'main',
    '--generate-notes'
)
if ($version -match '-') {
    $releaseArgs += '--prerelease'
}

& $script:GhExe @releaseArgs
if ($LASTEXITCODE -ne 0) {
    throw 'GitHub release creation failed.'
}

Write-Host "Published GitHub release $version using $package." -ForegroundColor Green
