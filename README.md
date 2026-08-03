# Best Bar v1.5.5

Best Bar replaces Geometry Dash's stock red progress fill with a large, shiny gold fill when a run becomes gold-qualified. A level already saved at 100% displays a complete gold bar immediately, while an active best-chase attempt fills gold from the left edge to the player's current percentage.

The BEST label uses the saved game percentage directly: an X is shown below 100%, and a golden animated checkmark is shown at 100%. The removed LET'S GO freeze feature is no longer present in the source, settings, or resources.

The rebuilt speedrun leaderboard uses its own green/cyan frame and dark-purple body so texture packs cannot turn the entire panel into a flat red box. Each run row includes rank, player, time, level name, difficulty, stars or moons, level ID, and VALID/CHEATED status. Local runs work without a server.

## Build

Run `BUILD.bat`. The project targets Geode SDK 5.8.2, Geometry Dash 2.2081, and Windows x64.

## Online leaderboard

The optional Cloudflare Worker is in `leaderboard-worker`. New databases use the v1.5 schema. For an existing pre-v1.5 database, run `MIGRATE_EXISTING_DB_TO_V1_5.bat` once before deploying the updated Worker.
