# v1.1.1

- Fixed the Geode 5.8.2 compile error caused by reading `m_fields` from a const method.
- Added `CREATE_GITHUB_AND_RELEASE_ONCE.bat`.
- The one-time script creates `Catchallcat5382/PermanentBest`, pushes all source files, waits for GitHub Actions, downloads the Windows `.geode`, publishes the v1.1.1 release, and optionally installs it.
- Switched the local build cache to `build-v111`.

# v1.1.0

- Added a permanent gold fill directly on the active normal-level progress bar.
- The gold fill stays at the saved best, then follows the player after the old best is reached.
- Completed normal levels can keep a fully gold progress bar.
- Added optional gold flash, shine sweep, and record banners.
- Added platformer-only Gold Speedrun mode with a custom pause-menu button.
- The button is gold on platformer levels and locked with a clear explanation on normal levels.
- Added a separate top-right speedrun timer, Golden Best time, run count, finishes, and failures.
- Gold Speedrun records are separate from normal platformer records.
- Saves the newest 20 Gold Speedrun completions and failed runs per level.
- Added Geode loader settings for every new visual and speedrun feature.
