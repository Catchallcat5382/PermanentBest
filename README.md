# Permanent Best

Repository target: `https://github.com/Catchallcat5382/PermanentBest`

A Windows Geode v5.8.2 mod for Geometry Dash 2.2081.

Permanent Best keeps a saved best marker on the active progress bar, updates it as soon as the old record is passed, and tracks fastest completed platformer times.

## Create the GitHub repository and first release

Run `CREATE_GITHUB_AND_RELEASE_ONCE.bat` once. It downloads portable Git/GitHub CLI into `.tools`, opens GitHub login when required, creates the public `Catchallcat5382/PermanentBest` repository, pushes the full source, waits for GitHub Actions, downloads the Windows build, publishes the current version as a release, and optionally installs it. The BAT can be deleted afterward.

## Build

Close Geometry Dash and run `BUILD.bat`.

The local builder uses the same project, release, workflow, and portable-tool structure as GD Path Solver. It builds `PermanentBest.dll`, creates `catchallcat5382.permanent-best.geode`, verifies the package, copies it to `releases/latest/PermanentBest.geode`, and installs it into the Geode mods folder.

## Settings

Open Geode, select **Permanent Best**, and use the gear button. Settings include the marker, BEST text, gold current-progress color, screen flash, label pulse, platformer timer, practice/test visibility, and custom best color.

The original `firee.goldenbest` mod changes the same percentage label. The included builder safely renames that package to `.disabled` during installation to prevent conflicts.
