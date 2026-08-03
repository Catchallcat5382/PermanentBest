$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

Initialize-PortableTools
Ensure-GitHubLogin
Ensure-RepositoryConnected

$currentVersion = ConvertTo-NormalizedModVersion -Version (Get-ModVersion)
$enteredVersion = Read-Host "Version to push [$currentVersion] (Enter keeps it; type PATCH to increment; any valid vX.Y.Z-suffix works)"

if ([string]::IsNullOrWhiteSpace($enteredVersion)) {
    $newVersion = Set-ModVersion -Version $currentVersion
}
elseif ($enteredVersion.Trim() -match '^(?i:patch|next)$') {
    $newVersion = Increment-PatchVersion
}
else {
    $newVersion = Set-ModVersion -Version $enteredVersion
}

Write-Host "Version: $newVersion" -ForegroundColor Cyan

$message = Read-Host 'Describe the update [Update Best Bar]'
if ([string]::IsNullOrWhiteSpace($message)) {
    $message = 'Update Best Bar'
}

Normalize-ProjectMetadata
$previousRunIDs = @(Get-WorkflowRunIDs)
$headSha = Commit-And-Push -Message "$message ($newVersion)"

if (-not $script:LastCommitCreated) {
    Write-Host 'No source changes required a new commit. Starting a fresh build manually...' -ForegroundColor Yellow
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
Install-GeodePackage -PackagePath $package

Write-Host "Built, pushed, and installed $newVersion from $script:Repository." -ForegroundColor Green
