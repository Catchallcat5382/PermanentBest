# Best Bar Leaderboard Worker v1.5.0

This optional Cloudflare Worker stores shared Gold Run leaderboard entries. The mod always keeps local entries when no server is configured.

## New deployment

1. Install Node.js and run `npm install`.
2. Run `npx wrangler login`.
3. Create the D1 database with `npx wrangler d1 create best-bar-leaderboard`.
4. Copy `wrangler.toml.example` to `wrangler.toml` and paste the database ID.
5. Run `npm run db:remote`.
6. Run `npm run deploy`.
7. Put the Worker base URL in Best Bar's Leaderboard API URL setting.

## Existing pre-v1.5 database

Run `MIGRATE_EXISTING_DB_TO_V1_5.bat` exactly once, then deploy. The migration adds difficulty, rewards, and platformer metadata columns.

Rows include level name, difficulty, stars/moons, platformer state, player, time, and VALID/CHEATED status. Invalid runs never replace valid ranked bests.
