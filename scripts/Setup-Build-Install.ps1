$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

Write-Host ''
Write-Host '============================================================' -ForegroundColor Magenta
Write-Host ' PERMANENT BEST - FIRST-TIME SETUP, BUILD, INSTALL' -ForegroundColor Magenta
Write-Host '============================================================' -ForegroundColor Magenta
Write-Host "Project:    $script:ProjectRoot" -ForegroundColor DarkGray
Write-Host "Repository: $script:Repository" -ForegroundColor DarkGray
Write-Host ''

Initialize-PortableTools
Ensure-GitHubLogin
Ensure-RepositoryConnected

$previousRunIDs = @(Get-WorkflowRunIDs)
$headSha = Commit-And-Push -Message 'Update Permanent Best metadata handling'

$runID = Wait-ForRunForCommit `
    -HeadSha $headSha `
    -PreviousRunIDs $previousRunIDs

Wait-ForRunSuccess -RunID $runID
$package = Download-BuildArtifact -RunID $runID
Install-GeodePackage -PackagePath $package -LaunchGame

Write-Host ''
Write-Host 'Setup is complete.' -ForegroundColor Green
Write-Host 'The mod stays in geode\mods and loads whenever Geometry Dash starts.' -ForegroundColor Green
