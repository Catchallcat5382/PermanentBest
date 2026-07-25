$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

Initialize-PortableTools
Ensure-GitHubLogin
Ensure-RepositoryConnected

$message = Read-Host 'Describe the update [Update Permanent Best]'
if ([string]::IsNullOrWhiteSpace($message)) {
    $message = 'Update Permanent Best'
}

$newVersion = Increment-PatchVersion
Write-Host "New version: $newVersion" -ForegroundColor Cyan

$previousRunIDs = @(Get-WorkflowRunIDs)
$headSha = Commit-And-Push -Message "$message ($newVersion)"

$runID = Wait-ForRunForCommit `
    -HeadSha $headSha `
    -PreviousRunIDs $previousRunIDs

Wait-ForRunSuccess -RunID $runID
$package = Download-BuildArtifact -RunID $runID
Install-GeodePackage -PackagePath $package

Write-Host "Built and installed $newVersion." -ForegroundColor Green
