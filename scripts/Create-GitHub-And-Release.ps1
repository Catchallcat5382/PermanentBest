$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

Write-Host ''
Write-Host '============================================================' -ForegroundColor Magenta
Write-Host ' PERMANENT BEST - CREATE GITHUB AND FIRST RELEASE' -ForegroundColor Magenta
Write-Host '============================================================' -ForegroundColor Magenta
Write-Host "Project:    $script:ProjectRoot" -ForegroundColor DarkGray
Write-Host "Repository: $script:Repository" -ForegroundColor DarkGray
Write-Host ''

Initialize-PortableTools
Ensure-GitHubLogin

$oldPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'SilentlyContinue'
    & $script:GhExe repo view $script:Repository *> $null
    $repositoryExists = ($LASTEXITCODE -eq 0)
}
finally {
    $ErrorActionPreference = $oldPreference
}

if (-not $repositoryExists) {
    Write-Host "Creating public repository $script:Repository..." -ForegroundColor Cyan
    & $script:GhExe repo create $script:Repository `
        --public `
        --description 'A Geode mod that permanently displays your best progress and adds a platformer Gold Speedrun mode.'

    if ($LASTEXITCODE -ne 0) {
        throw 'GitHub could not create the PermanentBest repository.'
    }
}
else {
    Write-Host 'The GitHub repository already exists; it will be updated.' -ForegroundColor Yellow
}

Ensure-RepositoryConnected

$version = Get-ModVersion
if ($version -notmatch '^v\d+\.\d+\.\d+$') {
    throw "Invalid mod version in mod.json: $version"
}

$headSha = Commit-And-Push -Message "Publish Permanent Best $version"
Write-Host "Pushed commit $headSha" -ForegroundColor Green

$runID = Wait-ForRunForCommit -HeadSha $headSha -PreviousRunIDs @()
Wait-ForRunSuccess -RunID $runID
$package = Download-BuildArtifact -RunID $runID

$tagLines = @(& $script:GitExe -C $script:ProjectRoot tag --list $version 2>&1)
$tagExists = $false
foreach ($line in $tagLines) {
    if ($line.ToString().Trim() -eq $version) {
        $tagExists = $true
        break
    }
}

if (-not $tagExists) {
    $code = Invoke-Git tag $version
    if ($code -ne 0) {
        throw "Could not create Git tag $version."
    }

    $code = Invoke-Git push origin $version
    if ($code -ne 0) {
        throw "Could not push Git tag $version."
    }
}

$oldPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'SilentlyContinue'
    & $script:GhExe release view $version --repo $script:Repository *> $null
    $releaseExists = ($LASTEXITCODE -eq 0)
}
finally {
    $ErrorActionPreference = $oldPreference
}

if ($releaseExists) {
    Write-Host "Release $version already exists; replacing its .geode asset." -ForegroundColor Yellow
    & $script:GhExe release upload $version $package `
        --repo $script:Repository `
        --clobber
}
else {
    & $script:GhExe release create $version `
        $package `
        --repo $script:Repository `
        --title "Permanent Best $version" `
        --generate-notes
}

if ($LASTEXITCODE -ne 0) {
    throw 'GitHub release publishing failed.'
}

Write-Host ''
Write-Host "Repository: https://github.com/$script:Repository" -ForegroundColor Cyan
Write-Host "Release:    https://github.com/$script:Repository/releases/tag/$version" -ForegroundColor Cyan
Write-Host "Package:    $package" -ForegroundColor Green
Write-Host ''

$install = Read-Host 'Install the GitHub-built mod into Geometry Dash now? [Y/n]'
if ([string]::IsNullOrWhiteSpace($install) -or $install -match '^(y|yes)$') {
    Install-GeodePackage -PackagePath $package
}

Write-Host ''
Write-Host 'GitHub repository and release are ready.' -ForegroundColor Green
