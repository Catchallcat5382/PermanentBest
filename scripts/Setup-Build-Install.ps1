$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

Write-Host ''
Write-Host '============================================================' -ForegroundColor Magenta
Write-Host ' BEST BAR - FIRST-TIME SETUP, BUILD, INSTALL' -ForegroundColor Magenta
Write-Host '============================================================' -ForegroundColor Magenta
Write-Host "Project:    $script:ProjectRoot" -ForegroundColor DarkGray
Write-Host ''

Initialize-PortableTools
Ensure-GitHubLogin
Ensure-RepositoryConnected
Write-Host "Repository: $script:Repository" -ForegroundColor DarkGray

$previousRunIDs = @(Get-WorkflowRunIDs)
$headSha = Commit-And-Push -Message 'Update Best Bar metadata handling'

if (-not $script:LastCommitCreated) {
    & $script:GhExe workflow run build.yml --repo $script:Repository --ref main
    if ($LASTEXITCODE -ne 0) {
        throw 'Could not start the GitHub Actions build.'
    }
}

$runID = Wait-ForRunForCommit `
    -HeadSha $headSha `
    -PreviousRunIDs $previousRunIDs

Wait-ForRunSuccess -RunID $runID
$package = Download-BuildArtifact -RunID $runID
Install-GeodePackage -PackagePath $package -LaunchGame

Write-Host ''
Write-Host 'Setup is complete.' -ForegroundColor Green
Write-Host 'The mod stays in geode\mods and loads whenever Geometry Dash starts.' -ForegroundColor Green
