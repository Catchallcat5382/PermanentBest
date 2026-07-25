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
$script:ModID = 'catchallcat5382.permanent-best'
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

    $headers = @{ 'User-Agent' = 'PermanentBest-Portable-Setup' }
    $zip = Join-Path $env:TEMP ("permanent-best-" + [Guid]::NewGuid().ToString('N') + '.zip')
    $extract = Join-Path $env:TEMP ("permanent-best-" + [Guid]::NewGuid().ToString('N'))

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
    $headers = @{ 'User-Agent' = 'PermanentBest-Portable-Setup' }

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

        $zip = Join-Path $env:TEMP ("permanent-best-gh-" + [Guid]::NewGuid().ToString('N') + '.zip')
        $extract = Join-Path $env:TEMP ("permanent-best-gh-" + [Guid]::NewGuid().ToString('N'))

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

    & $script:GitExe -C $script:ProjectRoot fetch origin main

    if ($LASTEXITCODE -eq 0) {
        $code = Invoke-Git reset origin/main

        if ($code -ne 0) {
            throw 'Could not align the local project with origin/main.'
        }
    }

    $code = Invoke-Git branch -M main

    if ($code -ne 0) {
        throw 'Could not set the branch to main.'
    }
}

function Get-ModVersion {
    $modPath = Join-Path $script:ProjectRoot 'mod.json'
    $mod = Get-Content -LiteralPath $modPath -Raw | ConvertFrom-Json
    return [string]$mod.version
}

function Set-ModVersion {
    param([Parameter(Mandatory = $true)][string]$Version)

    if ($Version -notmatch '^v(\d+)\.(\d+)\.(\d+)$') {
        throw 'The version must look like v0.2.1.'
    }

    $major = [int]$Matches[1]
    $minor = [int]$Matches[2]
    $patch = [int]$Matches[3]

    $modPath = Join-Path $script:ProjectRoot 'mod.json'
    $mod = Get-Content -LiteralPath $modPath -Raw | ConvertFrom-Json
    $mod.version = $Version
    $json = $mod | ConvertTo-Json -Depth 20
    Write-Utf8NoBom -Path $modPath -Content ($json + [Environment]::NewLine)

    $cmakePath = Join-Path $script:ProjectRoot 'CMakeLists.txt'
    $cmake = Get-Content -LiteralPath $cmakePath -Raw
    $cmake = [regex]::Replace(
        $cmake,
        'project\(PermanentBest VERSION \d+\.\d+\.\d+\)',
        "project(PermanentBest VERSION $major.$minor.$patch)"
    )
    Write-Utf8NoBom -Path $cmakePath -Content $cmake


}

function Increment-PatchVersion {
    $current = Get-ModVersion

    if ($current -notmatch '^v(\d+)\.(\d+)\.(\d+)$') {
        throw "Cannot increment invalid version: $current"
    }

    $next = "v$($Matches[1]).$($Matches[2]).$([int]$Matches[3] + 1)"
    Set-ModVersion -Version $next
    return $next
}

function Get-WorkflowRunIDs {
    $lines = @(& $script:GhExe run list `
        --repo $script:Repository `
        --workflow build.yml `
        --limit 50 `
        --json databaseId `
        --jq '.[].databaseId' 2>&1)

    if ($LASTEXITCODE -ne 0) {
        throw "Could not list GitHub Actions runs.`n$($lines -join "`n")"
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
            throw "Could not find the build for commit $HeadSha.`n$($lines -join "`n")"
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

    $work = Join-Path $env:TEMP ("permanent-best-package-" + [Guid]::NewGuid().ToString('N'))
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

    $versionedPackage = Join-Path $versionDir "PermanentBest-$Version.geode"
    $latestPackage = Join-Path $latestDir 'PermanentBest.geode'

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

    $oldGolden = Join-Path $modsDir 'firee.goldenbest.geode'
    $oldGoldenDisabled = Join-Path $modsDir 'firee.goldenbest.geode.disabled'
    if (Test-Path -LiteralPath $oldGolden) {
        Remove-Item -LiteralPath $oldGoldenDisabled -Force -ErrorAction SilentlyContinue
        Move-Item -LiteralPath $oldGolden -Destination $oldGoldenDisabled -Force
        Write-Host 'Disabled firee.goldenbest to prevent progress-label conflicts.' -ForegroundColor Yellow
    }
    Remove-Item -LiteralPath (Join-Path $gameRoot 'geode\unzipped\firee.goldenbest') -Recurse -Force -ErrorAction SilentlyContinue

    Get-ChildItem -LiteralPath $modsDir -File -ErrorAction SilentlyContinue |
        Where-Object {
            $_.Name -like 'PermanentBest*.geode' -or
            $_.Name -like 'catchallcat5382.permanent-best*.geode'
        } |
        Remove-Item -Force -ErrorAction SilentlyContinue

    $cacheDir = Join-Path $gameRoot "geode\unzipped\$($script:ModID)"
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

    & $script:GhExe run download $RunID `
        --repo $script:Repository `
        --name 'PermanentBest-Windows' `
        --dir $download

    if ($LASTEXITCODE -ne 0) {
        throw 'Could not download the Windows build artifact.'
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
    }
    elseif ($diffExit -ne 0) {
        throw 'Git could not inspect staged changes.'
    }

    $code = Invoke-Git push -u origin main

    if ($code -ne 0) {
        throw 'Git push failed.'
    }

    $headLines = @(& $script:GitExe -C $script:ProjectRoot rev-parse HEAD 2>&1)

    if ($LASTEXITCODE -ne 0 -or $headLines.Count -eq 0) {
        throw 'Could not read the current commit.'
    }

    return ($headLines | Select-Object -First 1).ToString().Trim()
}
