$ErrorActionPreference = 'Stop'

if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}

$script:ProjectRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$script:ToolsRoot = Join-Path $script:ProjectRoot '.tools'
$script:GitRoot = Join-Path $script:ToolsRoot 'git'
$script:GhRoot = Join-Path $script:ToolsRoot 'gh'
$script:GitExe = Join-Path $script:GitRoot 'cmd\git.exe'
$script:GhExe = Join-Path $script:GhRoot 'gh.exe'
$script:GitConfigPath = Join-Path $script:ToolsRoot 'portable-gitconfig'

$script:Repository = 'Catchallcat5382/PermanentBest'
$script:RepositoryUrl = 'https://github.com/Catchallcat5382/PermanentBest.git'
$script:RepositoryConfigPath = Join-Path $script:ProjectRoot '.bestbar-repository'
$script:ModID = 'catchallcat5382.best-bar'
$script:DefaultGameRoot = 'F:\SteamLibrary\steamapps\common\Geometry Dash'


function Write-Utf8NoBom {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$Content
    )

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $encoding)
}

function Normalize-ProjectMetadata {
    $modPath = Join-Path $script:ProjectRoot 'mod.json'
    $cmakePath = Join-Path $script:ProjectRoot 'CMakeLists.txt'

    if (-not (Test-Path -LiteralPath $modPath)) {
        throw 'mod.json is missing.'
    }

    $modText = [System.IO.File]::ReadAllText($modPath)
    $mod = $modText | ConvertFrom-Json

    if (-not [string]::IsNullOrWhiteSpace($script:Repository)) {
        $repositoryWeb = "https://github.com/$($script:Repository)"
        if (-not $mod.links) {
            $mod | Add-Member -MemberType NoteProperty -Name links -Value ([PSCustomObject]@{})
        }
        if (-not $mod.links.PSObject.Properties['homepage']) {
            $mod.links | Add-Member -MemberType NoteProperty -Name homepage -Value $repositoryWeb
        }
        else {
            $mod.links.homepage = $repositoryWeb
        }
        if (-not $mod.links.PSObject.Properties['source']) {
            $mod.links | Add-Member -MemberType NoteProperty -Name source -Value $repositoryWeb
        }
        else {
            $mod.links.source = $repositoryWeb
        }
        if (-not $mod.issues) {
            $mod | Add-Member -MemberType NoteProperty -Name issues -Value ([PSCustomObject]@{})
        }
        if (-not $mod.issues.PSObject.Properties['url']) {
            $mod.issues | Add-Member -MemberType NoteProperty -Name url -Value "$repositoryWeb/issues/new"
        }
        else {
            $mod.issues.url = "$repositoryWeb/issues/new"
        }
    }

    if ([string]$mod.geode -match '^v(.+)$') {
        $mod.geode = $Matches[1]
    }

    $json = $mod | ConvertTo-Json -Depth 20
    Write-Utf8NoBom -Path $modPath -Content ($json + [Environment]::NewLine)

    # Parse the exact bytes that will be pushed. Geode's cloud parser rejects a UTF-8 BOM.
    $bytes = [System.IO.File]::ReadAllBytes($modPath)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        throw 'mod.json still contains a UTF-8 BOM.'
    }

    $null = ([System.IO.File]::ReadAllText($modPath) | ConvertFrom-Json)

    if (Test-Path -LiteralPath $cmakePath) {
        $cmake = [System.IO.File]::ReadAllText($cmakePath)
        Write-Utf8NoBom -Path $cmakePath -Content $cmake
    }
}

function Download-And-ExpandZip {
    param(
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $headers = @{ 'User-Agent' = 'BestBar-Portable-Setup' }
    $zip = Join-Path $env:TEMP ("best-bar-" + [Guid]::NewGuid().ToString('N') + '.zip')
    $extract = Join-Path $env:TEMP ("best-bar-" + [Guid]::NewGuid().ToString('N'))

    try {
        Write-Host "Downloading $Url" -ForegroundColor DarkGray
        Invoke-WebRequest -UseBasicParsing -Headers $headers -Uri $Url -OutFile $zip
        New-Item -ItemType Directory -Path $extract -Force | Out-Null
        Expand-Archive -LiteralPath $zip -DestinationPath $extract -Force

        if (Test-Path -LiteralPath $Destination) {
            Remove-Item -LiteralPath $Destination -Recurse -Force
        }

        New-Item -ItemType Directory -Path $Destination -Force | Out-Null

        Get-ChildItem -LiteralPath $extract -Force | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $Destination -Recurse -Force
        }
    }
    finally {
        Remove-Item -LiteralPath $zip -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $extract -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Initialize-PortableTools {
    New-Item -ItemType Directory -Path $script:ToolsRoot -Force | Out-Null
    $headers = @{ 'User-Agent' = 'BestBar-Portable-Setup' }

    if (-not (Test-Path -LiteralPath $script:GitExe)) {
        Write-Host 'Downloading portable Git. Nothing is installed system-wide.' -ForegroundColor Cyan
        $release = Invoke-RestMethod -Headers $headers -Uri 'https://api.github.com/repos/git-for-windows/git/releases/latest'
        $asset = $release.assets |
            Where-Object {
                $_.name -match '^MinGit-.*-64-bit\.zip$' -and
                $_.name -notmatch 'busybox'
            } |
            Select-Object -First 1

        if (-not $asset) {
            throw 'Could not locate the current 64-bit MinGit ZIP.'
        }

        Download-And-ExpandZip -Url $asset.browser_download_url -Destination $script:GitRoot
    }

    if (-not (Test-Path -LiteralPath $script:GhExe)) {
        Write-Host 'Downloading portable GitHub CLI. Nothing is installed system-wide.' -ForegroundColor Cyan
        $release = Invoke-RestMethod -Headers $headers -Uri 'https://api.github.com/repos/cli/cli/releases/latest'
        $asset = $release.assets |
            Where-Object { $_.name -match '^gh_.*_windows_amd64\.zip$' } |
            Select-Object -First 1

        if (-not $asset) {
            throw 'Could not locate the current Windows GitHub CLI ZIP.'
        }

        $zip = Join-Path $env:TEMP ("best-bar-gh-" + [Guid]::NewGuid().ToString('N') + '.zip')
        $extract = Join-Path $env:TEMP ("best-bar-gh-" + [Guid]::NewGuid().ToString('N'))

        try {
            Invoke-WebRequest -UseBasicParsing -Headers $headers -Uri $asset.browser_download_url -OutFile $zip
            Expand-Archive -LiteralPath $zip -DestinationPath $extract -Force

            $downloaded = Get-ChildItem -LiteralPath $extract -Recurse -Filter gh.exe |
                Select-Object -First 1

            if (-not $downloaded) {
                throw 'The GitHub CLI ZIP did not contain gh.exe.'
            }

            New-Item -ItemType Directory -Path $script:GhRoot -Force | Out-Null
            Copy-Item -LiteralPath $downloaded.FullName -Destination $script:GhExe -Force
        }
        finally {
            Remove-Item -LiteralPath $zip -Force -ErrorAction SilentlyContinue
            Remove-Item -LiteralPath $extract -Recurse -Force -ErrorAction SilentlyContinue
        }
    }

    $gitCmd = Split-Path -Parent $script:GitExe

    if (($env:PATH -split ';') -notcontains $gitCmd) {
        $env:PATH = "$gitCmd;$env:PATH"
    }

    $env:GIT_CONFIG_GLOBAL = $script:GitConfigPath

    $normalizedRoot = $script:ProjectRoot.Replace('\', '/')
    $safeLines = @(& $script:GitExe config --global --get-all safe.directory 2>$null)
    $safe = @(
        $safeLines |
            ForEach-Object { $_.ToString().Trim() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    if ($safe -notcontains $normalizedRoot) {
        & $script:GitExe config --global --add safe.directory $normalizedRoot

        if ($LASTEXITCODE -ne 0) {
            throw 'Could not mark this project as a trusted Git directory.'
        }
    }
}

function Invoke-Git {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments)

    & $script:GitExe -C $script:ProjectRoot @Arguments | Out-Host
    return [int]$LASTEXITCODE
}

function Invoke-GitQuiet {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments)

    $oldPreference = $ErrorActionPreference

    try {
        # Windows PowerShell can convert native stderr into a PowerShell error
        # when ErrorActionPreference is Stop. Silence it deliberately for probes.
        $ErrorActionPreference = 'SilentlyContinue'
        & $script:GitExe -C $script:ProjectRoot @Arguments *> $null
        return [int]$LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $oldPreference
    }
}

function Test-RemoteBranchExists {
    param([Parameter(Mandatory = $true)][string]$Branch)

    $code = Invoke-GitQuiet ls-remote --exit-code --heads origin "refs/heads/$Branch"
    return ($code -eq 0)
}

function Ensure-GitHubLogin {
    $oldPreference = $ErrorActionPreference

    try {
        $ErrorActionPreference = 'SilentlyContinue'
        & $script:GhExe auth status *> $null
        $loggedIn = ($LASTEXITCODE -eq 0)
    }
    finally {
        $ErrorActionPreference = $oldPreference
    }

    if (-not $loggedIn) {
        Write-Host 'A browser will open for GitHub login.' -ForegroundColor Yellow
        & $script:GhExe auth login --web --git-protocol https

        if ($LASTEXITCODE -ne 0) {
            throw 'GitHub login failed.'
        }
    }

    & $script:GhExe auth setup-git

    if ($LASTEXITCODE -ne 0) {
        throw 'GitHub CLI could not configure portable Git authentication.'
    }

    $loginLines = @(& $script:GhExe api user --jq .login 2>&1)

    if ($LASTEXITCODE -ne 0 -or $loginLines.Count -eq 0) {
        throw 'Could not read the signed-in GitHub account.'
    }

    $login = ($loginLines | Select-Object -First 1).ToString().Trim()

    if ($login -ine 'Catchallcat5382') {
        throw "GitHub is signed in as '$login'. Sign in as Catchallcat5382."
    }
}

function ConvertTo-RepositorySlug {
    param([AllowNull()][AllowEmptyString()][string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $null
    }

    $text = $Value.Trim().Replace('\', '/')
    $text = $text -replace '^https?://github\.com/', ''
    $text = $text -replace '^ssh://git@github\.com/', ''
    $text = $text -replace '^git@github\.com:', ''
    $text = $text.TrimEnd('/')
    $text = $text -replace '\.git$', ''

    if ($text -match '^([A-Za-z0-9_.-]+)/([A-Za-z0-9_.-]+)$') {
        return "$($Matches[1])/$($Matches[2])"
    }

    return $null
}

function Test-GitHubRepository {
    param([Parameter(Mandatory = $true)][string]$Repository)

    $oldPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'SilentlyContinue'
        & $script:GhExe repo view $Repository --json nameWithOwner --jq .nameWithOwner *> $null
        return ($LASTEXITCODE -eq 0)
    }
    finally {
        $ErrorActionPreference = $oldPreference
    }
}

function Set-RepositoryContext {
    param([Parameter(Mandatory = $true)][string]$Repository)

    $slug = ConvertTo-RepositorySlug -Value $Repository
    if ([string]::IsNullOrWhiteSpace($slug)) {
        throw "Invalid GitHub repository: $Repository. Use owner/name or a GitHub URL."
    }

    $script:Repository = $slug
    $script:RepositoryUrl = "https://github.com/$slug.git"
    Write-Utf8NoBom -Path $script:RepositoryConfigPath -Content ($slug + [Environment]::NewLine)
}

function Resolve-Repository {
    $expectedRepository = 'Catchallcat5382/PermanentBest'

    if (-not (Test-GitHubRepository -Repository $expectedRepository)) {
        throw "Required GitHub repository '$expectedRepository' was not found or is not accessible. The script will never create a replacement repository."
    }

    Set-RepositoryContext -Repository $expectedRepository
}

function Ensure-RepositoryConnected {
    if (-not (Test-Path -LiteralPath (Join-Path $script:ProjectRoot '.git'))) {
        $code = Invoke-Git init -b main

        if ($code -ne 0) {
            throw 'git init failed.'
        }
    }

    foreach ($setting in @(
        @('core.autocrlf', 'false'),
        @('core.filemode', 'false'),
        @('user.name', 'Catchallcat5382'),
        @('user.email', 'Catchallcat5382@users.noreply.github.com')
    )) {
        $code = Invoke-Git config --local $setting[0] $setting[1]

        if ($code -ne 0) {
            throw "Could not configure $($setting[0])."
        }
    }

    Resolve-Repository

    $remoteNames = @(
        & $script:GitExe -C $script:ProjectRoot remote 2>&1 |
            ForEach-Object { $_.ToString().Trim() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    if ($remoteNames -contains 'origin') {
        $code = Invoke-Git remote set-url origin $script:RepositoryUrl
    }
    else {
        $code = Invoke-Git remote add origin $script:RepositoryUrl
    }

    if ($code -ne 0) {
        throw 'Could not configure the GitHub origin remote.'
    }

    $code = Invoke-Git branch -M main
    if ($code -ne 0) {
        throw 'Could not set the branch to main.'
    }

    # Fetch only when the remote already has a main branch. Newly-created
    # repositories are empty, so attempting to fetch origin/main would fail.
    if (Test-RemoteBranchExists -Branch 'main') {
        $fetchCode = Invoke-GitQuiet fetch origin main
        if ($fetchCode -ne 0) {
            throw 'Could not fetch the existing origin/main branch.'
        }
    }
    else {
        Write-Host 'Remote repository is empty. The first push will create main.' -ForegroundColor DarkGray
    }

    Normalize-ProjectMetadata
    Write-Host "Repository: $script:Repository" -ForegroundColor DarkGray
}

function Get-ModVersion {
    $modPath = Join-Path $script:ProjectRoot 'mod.json'
    $mod = Get-Content -LiteralPath $modPath -Raw | ConvertFrom-Json
    return [string]$mod.version
}

function ConvertTo-NormalizedModVersion {
    param([Parameter(Mandatory = $true)][string]$Version)

    $value = $Version.Trim()
    $pattern = '^[vV]?(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*))?(?:\+([0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*))?$'

    if ($value -notmatch $pattern) {
        throw "Invalid version '$Version'. Use vMAJOR.MINOR.PATCH with an optional suffix, such as v2.0.0, v2.0.0-beta.4, or v2.0.0-rc.1+test."
    }

    $normalized = "v$($Matches[1]).$($Matches[2]).$($Matches[3])"
    if (-not [string]::IsNullOrWhiteSpace($Matches[4])) {
        $normalized += "-$($Matches[4])"
    }
    if (-not [string]::IsNullOrWhiteSpace($Matches[5])) {
        $normalized += "+$($Matches[5])"
    }

    return $normalized
}

function Set-ModVersion {
    param([Parameter(Mandatory = $true)][string]$Version)

    $normalized = ConvertTo-NormalizedModVersion -Version $Version
    $null = $normalized -match '^v(\d+)\.(\d+)\.(\d+)'
    $major = [int]$Matches[1]
    $minor = [int]$Matches[2]
    $patch = [int]$Matches[3]

    $modPath = Join-Path $script:ProjectRoot 'mod.json'
    $mod = Get-Content -LiteralPath $modPath -Raw | ConvertFrom-Json
    $mod.version = $normalized
    $json = $mod | ConvertTo-Json -Depth 20
    Write-Utf8NoBom -Path $modPath -Content ($json + [Environment]::NewLine)

    $cmakePath = Join-Path $script:ProjectRoot 'CMakeLists.txt'
    if (Test-Path -LiteralPath $cmakePath) {
        $cmake = Get-Content -LiteralPath $cmakePath -Raw
        $cmake = [regex]::Replace(
            $cmake,
            'project\(BestBar VERSION \d+\.\d+\.\d+\)',
            "project(BestBar VERSION $major.$minor.$patch)"
        )
        Write-Utf8NoBom -Path $cmakePath -Content $cmake
    }

    return $normalized
}

function Increment-PatchVersion {
    $current = ConvertTo-NormalizedModVersion -Version (Get-ModVersion)
    $null = $current -match '^v(\d+)\.(\d+)\.(\d+)(?:-([^+]+))?(?:\+.+)?$'
    $major = [int]$Matches[1]
    $minor = [int]$Matches[2]
    $patch = [int]$Matches[3]
    $prerelease = [string]$Matches[4]

    if (-not [string]::IsNullOrWhiteSpace($prerelease)) {
        if ($prerelease -match '^(.*\.)(\d+)$') {
            $next = "v$major.$minor.$patch-$($Matches[1])$([int]$Matches[2] + 1)"
        }
        else {
            $next = "v$major.$minor.$patch-$prerelease.1"
        }
    }
    else {
        $next = "v$major.$minor.$($patch + 1)"
    }

    return Set-ModVersion -Version $next
}

function Get-WorkflowRunIDs {
    $lines = @(& $script:GhExe run list `
        --repo $script:Repository `
        --workflow build.yml `
        --limit 50 `
        --json databaseId `
        --jq '.[].databaseId' 2>&1)

    if ($LASTEXITCODE -ne 0) {
        Write-Host 'No previous build workflow runs were found yet. This is normal for a new repository.' -ForegroundColor DarkGray
        return @()
    }

    $ids = New-Object System.Collections.Generic.List[long]

    foreach ($line in $lines) {
        $text = $line.ToString().Trim()

        if ($text -match '^\d+$') {
            $ids.Add([long]$text)
        }
    }

    return $ids.ToArray()
}

function Wait-ForRunForCommit {
    param(
        [Parameter(Mandatory = $true)][string]$HeadSha,
        [long[]]$PreviousRunIDs = @(),
        [int]$TimeoutMinutes = 15
    )

    $deadline = (Get-Date).AddMinutes($TimeoutMinutes)
    Write-Host 'Waiting for the matching GitHub Actions build...' -ForegroundColor Cyan

    while ((Get-Date) -lt $deadline) {
        $lines = @(& $script:GhExe run list `
            --repo $script:Repository `
            --workflow build.yml `
            --commit $HeadSha `
            --limit 10 `
            --json databaseId `
            --jq '.[].databaseId' 2>&1)

        if ($LASTEXITCODE -ne 0) {
            # GitHub can take a few seconds to register build.yml after the
            # first push to a new repository. Keep polling instead of failing.
            Start-Sleep -Seconds 5
            continue
        }

        foreach ($line in $lines) {
            $text = $line.ToString().Trim()

            if ($text -match '^\d+$') {
                $id = [long]$text

                if ($PreviousRunIDs -notcontains $id) {
                    return $id
                }
            }
        }

        Start-Sleep -Seconds 5
    }

    throw 'Timed out waiting for the matching GitHub Actions run.'
}

function Save-BuildFailureLog {
    param([Parameter(Mandatory = $true)][long]$RunID)

    $logPath = Join-Path $script:ProjectRoot 'build-failure.txt'
    $lines = @(& $script:GhExe run view $RunID --repo $script:Repository --log-failed 2>&1)

    if ($LASTEXITCODE -ne 0 -or $lines.Count -eq 0) {
        $lines = @(& $script:GhExe run view $RunID --repo $script:Repository --log 2>&1)
    }

    Write-Utf8NoBom -Path $logPath -Content (($lines -join [Environment]::NewLine) + [Environment]::NewLine)

    Write-Host ''
    Write-Host "Build log: $logPath" -ForegroundColor Yellow
    $lines | Select-Object -Last 100 | ForEach-Object { Write-Host $_ }
}

function Wait-ForRunSuccess {
    param([Parameter(Mandatory = $true)][long]$RunID)

    Write-Host "Watching GitHub Actions run $RunID..." -ForegroundColor Cyan
    & $script:GhExe run watch $RunID --repo $script:Repository --exit-status

    if ($LASTEXITCODE -ne 0) {
        Save-BuildFailureLog -RunID $RunID
        throw 'The cloud build failed.'
    }
}

function Test-GeodePackage {
    param([Parameter(Mandatory = $true)][string]$PackagePath)

    $work = Join-Path $env:TEMP ("best-bar-package-" + [Guid]::NewGuid().ToString('N'))
    $zip = Join-Path $work 'package.zip'
    $extract = Join-Path $work 'extract'

    try {
        New-Item -ItemType Directory -Path $work -Force | Out-Null
        Copy-Item -LiteralPath $PackagePath -Destination $zip -Force
        Expand-Archive -LiteralPath $zip -DestinationPath $extract -Force

        $embeddedPath = Join-Path $extract 'mod.json'

        if (-not (Test-Path -LiteralPath $embeddedPath)) {
            throw 'The built .geode file does not contain mod.json.'
        }

        $embedded = Get-Content -LiteralPath $embeddedPath -Raw | ConvertFrom-Json

        if ([string]$embedded.id -ne $script:ModID) {
            throw "Wrong mod package was downloaded. Found ID: $($embedded.id)"
        }

        return [string]$embedded.version
    }
    finally {
        Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Store-PackageInReleases {
    param(
        [Parameter(Mandatory = $true)][string]$PackagePath,
        [Parameter(Mandatory = $true)][string]$Version
    )

    $releaseRoot = Join-Path $script:ProjectRoot 'releases'
    $versionDir = Join-Path $releaseRoot "versions\$Version"
    $latestDir = Join-Path $releaseRoot 'latest'

    New-Item -ItemType Directory -Path $versionDir -Force | Out-Null
    New-Item -ItemType Directory -Path $latestDir -Force | Out-Null

    $versionedPackage = Join-Path $versionDir "BestBar-$Version.geode"
    $latestPackage = Join-Path $latestDir 'BestBar.geode'

    Copy-Item -LiteralPath $PackagePath -Destination $versionedPackage -Force
    Copy-Item -LiteralPath $PackagePath -Destination $latestPackage -Force

    Set-Content -LiteralPath (Join-Path $latestDir 'version.txt') -Value $Version -Encoding ASCII

    Write-Host "Saved version: $versionedPackage" -ForegroundColor Green
    Write-Host "Updated latest: $latestPackage" -ForegroundColor Green

    return $latestPackage
}

function Get-GameRoot {
    $gameRoot = $script:DefaultGameRoot
    $gameExe = Join-Path $gameRoot 'GeometryDash.exe'

    if (-not (Test-Path -LiteralPath $gameExe)) {
        $entered = Read-Host "Geometry Dash folder [$gameRoot]"

        if (-not [string]::IsNullOrWhiteSpace($entered)) {
            $gameRoot = $entered.Trim('"')
        }
    }

    $gameExe = Join-Path $gameRoot 'GeometryDash.exe'

    if (-not (Test-Path -LiteralPath $gameExe)) {
        throw "GeometryDash.exe was not found in: $gameRoot"
    }

    return $gameRoot
}

function Install-GeodePackage {
    param(
        [Parameter(Mandatory = $true)][string]$PackagePath,
        [switch]$LaunchGame
    )

    $embeddedVersion = Test-GeodePackage -PackagePath $PackagePath
    $gameRoot = Get-GameRoot
    $gameExe = Join-Path $gameRoot 'GeometryDash.exe'

    while (Get-Process -Name GeometryDash -ErrorAction SilentlyContinue) {
        Write-Host 'Close Geometry Dash, then press Enter to continue installation.' -ForegroundColor Yellow
        Read-Host | Out-Null
    }

    $modsDir = Join-Path $gameRoot 'geode\mods'
    New-Item -ItemType Directory -Path $modsDir -Force | Out-Null

    Get-ChildItem -LiteralPath $modsDir -File -ErrorAction SilentlyContinue |
        Where-Object {
            $_.Name -like 'BestBar*.geode' -or
            $_.Name -like 'catchallcat5382.*best*.geode'
        } |
        Remove-Item -Force -ErrorAction SilentlyContinue

    $unzippedDir = Join-Path $gameRoot 'geode\unzipped'
    Get-ChildItem -LiteralPath $unzippedDir -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like 'catchallcat5382.*best*' } |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

    $cacheDir = Join-Path $unzippedDir $script:ModID
    Remove-Item -LiteralPath $cacheDir -Recurse -Force -ErrorAction SilentlyContinue

    $destination = Join-Path $modsDir "$($script:ModID).geode"
    Copy-Item -LiteralPath $PackagePath -Destination $destination -Force

    if (-not (Test-Path -LiteralPath $destination)) {
        throw 'The mod package was not copied into the Geode mods folder.'
    }

    $latestDir = Join-Path $script:ProjectRoot 'releases\latest'
    Write-Utf8NoBom -Path (Join-Path $latestDir 'installed-path.txt') -Content ($destination + [Environment]::NewLine)

    Write-Host "Installed $embeddedVersion to: $destination" -ForegroundColor Green

    if ($LaunchGame) {
        $process = Start-Process -FilePath $gameExe -WorkingDirectory $gameRoot -PassThru
        Start-Sleep -Seconds 5

        if ($process.HasExited) {
            throw "Geometry Dash closed immediately with exit code $($process.ExitCode)."
        }

        Write-Host 'Geometry Dash started successfully.' -ForegroundColor Green
    }
}

function Download-BuildArtifact {
    param([Parameter(Mandatory = $true)][long]$RunID)

    $download = Join-Path $script:ProjectRoot 'dist\github'

    if (Test-Path -LiteralPath $download) {
        Remove-Item -LiteralPath $download -Recurse -Force
    }

    New-Item -ItemType Directory -Path $download -Force | Out-Null

    $artifactNames = @('BestBar-Windows', 'PermanentBest-Windows')
    $downloaded = $false

    foreach ($artifactName in $artifactNames) {
        & $script:GhExe run download $RunID `
            --repo $script:Repository `
            --name $artifactName `
            --dir $download

        if ($LASTEXITCODE -eq 0) {
            $downloaded = $true
            break
        }
    }

    if (-not $downloaded) {
        throw 'Could not download the Windows build artifact from PermanentBest.'
    }

    $package = Get-ChildItem -LiteralPath $download -Recurse -Filter *.geode |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $package) {
        throw 'The downloaded artifact did not contain a .geode file.'
    }

    $version = Test-GeodePackage -PackagePath $package.FullName
    $latestPackage = Store-PackageInReleases -PackagePath $package.FullName -Version $version

    return $latestPackage
}

function Commit-And-Push {
    param([Parameter(Mandatory = $true)][string]$Message)

    $script:LastCommitCreated = $false
    Normalize-ProjectMetadata

    $code = Invoke-Git add --all

    if ($code -ne 0) {
        throw 'Could not stage project files.'
    }

    & $script:GitExe -C $script:ProjectRoot diff --cached --quiet --
    $diffExit = $LASTEXITCODE

    if ($diffExit -eq 1) {
        $code = Invoke-Git commit -m $Message

        if ($code -ne 0) {
            throw 'Git commit failed.'
        }
        $script:LastCommitCreated = $true
    }
    elseif ($diffExit -ne 0) {
        throw 'Git could not inspect staged changes.'
    }

    $code = Invoke-Git push -u origin main

    if ($code -ne 0) {
        if (-not (Test-RemoteBranchExists -Branch 'main')) {
            throw 'Git could not create the first origin/main branch. Check the repository permission and origin URL shown above.'
        }

        Write-Host 'The first push was rejected. Trying a safe rebase against origin/main...' -ForegroundColor Yellow
        $pullCode = Invoke-Git pull --rebase origin main --allow-unrelated-histories
        if ($pullCode -ne 0) {
            throw 'Git push failed, and the project could not be rebased onto origin/main.'
        }

        $code = Invoke-Git push -u origin main
        if ($code -ne 0) {
            throw 'Git push failed.'
        }
    }

    $headLines = @(& $script:GitExe -C $script:ProjectRoot rev-parse HEAD 2>&1)

    if ($LASTEXITCODE -ne 0 -or $headLines.Count -eq 0) {
        throw 'Could not read the current commit.'
    }

    return ($headLines | Select-Object -First 1).ToString().Trim()
}
