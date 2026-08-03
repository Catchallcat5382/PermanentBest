CREATE TABLE IF NOT EXISTS runs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    run_nonce TEXT NOT NULL UNIQUE,
    level_id TEXT NOT NULL,
    level_name TEXT NOT NULL,
    difficulty TEXT NOT NULL DEFAULT 'UNKNOWN',
    rewards INTEGER NOT NULL DEFAULT 0 CHECK (rewards >= 0),
    platformer INTEGER NOT NULL DEFAULT 1 CHECK (platformer IN (0, 1)),
    player TEXT NOT NULL,
    player_id TEXT NOT NULL,
    time_ms INTEGER NOT NULL CHECK (time_ms > 0),
    valid INTEGER NOT NULL DEFAULT 0 CHECK (valid IN (0, 1)),
    status TEXT NOT NULL,
    reason TEXT NOT NULL DEFAULT '',
    safe_mode INTEGER NOT NULL DEFAULT 0 CHECK (safe_mode IN (0, 1)),
    mod_version TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_runs_valid_time ON runs(valid, time_ms);
CREATE INDEX IF NOT EXISTS idx_runs_level_valid_time ON runs(level_id, valid, time_ms);
CREATE INDEX IF NOT EXISTS idx_runs_player_level ON runs(player_id, level_id);
