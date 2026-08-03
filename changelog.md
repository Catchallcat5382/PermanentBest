# Changelog

## v1.5.5

- Removed the LET'S GO freeze setting, source code, and sprite resource.
- Rebuilt gold-fill layout around the full stock progress-bar geometry instead of the changing current-fill bounding box.
- Saved 100% completion now forces a complete gold bar immediately.
- Increased gold fill height, glow strength, and clipped shine visibility.
- Rebuilt the leaderboard with a green/cyan frame, dark-purple body, compact ranked rows, and a proper empty state.
- Added level difficulty, stars/moons, level ID, and platformer/classic metadata to local and online leaderboard entries.
- Kept backward compatibility with existing five-field local leaderboard saves.

## v1.4.0
- Replaced the stock red progress fill itself with an exact-size gold fill after passing attempt-start best; removed the separate oversized gold bar.
- Restored gold glow, shine, ambient sparkles, burst VFX, and gold diamond effects around the real fill.
- Rebuilt and recentered the BEST X; completion still animates into the golden checkmark.
- Rebuilt the leaderboard as one centered Geometry Dash-style green/teal/brown panel with seven larger rows and clear VALID/CHEATED badges.
- Increased the main-menu leaderboard icon size for better readability.

## v1.3.1
- Full thick gold progress overlay replaces the thin strip.
- Gold appears only for active Gold Run, passed original attempt best, or 100%.
- Moved completion state beside BEST: normal X before completion, gold check after 100%.
- Added themed main-menu leaderboard button and auto-refreshing leaderboard page.
- Added persistent local runs and optional shared Worker/D1 backend.
- Added Safe Mode, major-assist detection, runtime time-scale detection, and CHEATED / NOT VALID results.
- Invalid runs cannot overwrite Gold Best.
