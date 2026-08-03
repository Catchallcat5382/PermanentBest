interface Env {
  DB: D1Database;
}

interface RunPayload {
  levelId: string;
  levelName: string;
  difficulty: string;
  rewards: number;
  platformer: boolean;
  player: string;
  playerId: string;
  timeMs: number;
  valid: boolean;
  status: string;
  reason?: string;
  safeMode: boolean;
  modVersion: string;
  runNonce: string;
}

const corsHeaders: Record<string, string> = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Headers": "Content-Type",
  "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
  "Cache-Control": "no-store"
};

function json(data: unknown, status = 200): Response {
  return new Response(JSON.stringify(data), {
    status,
    headers: { "Content-Type": "application/json; charset=utf-8", ...corsHeaders }
  });
}

function clean(value: unknown, max: number): string {
  return String(value ?? "")
    .replace(/[|;\r\n\u0000-\u001f]/g, " ")
    .trim()
    .slice(0, max);
}

function readPayload(input: unknown): RunPayload | null {
  if (!input || typeof input !== "object") return null;
  const body = input as Record<string, unknown>;
  const payload: RunPayload = {
    levelId: clean(body.levelId, 96),
    levelName: clean(body.levelName, 80),
    difficulty: clean(body.difficulty, 40) || "UNKNOWN",
    rewards: Math.max(0, Math.min(100, Math.trunc(Number(body.rewards) || 0))),
    platformer: body.platformer !== false,
    player: clean(body.player, 24),
    playerId: clean(body.playerId, 96),
    timeMs: Number(body.timeMs),
    valid: body.valid === true,
    status: clean(body.status, 40),
    reason: clean(body.reason, 200),
    safeMode: body.safeMode === true,
    modVersion: clean(body.modVersion, 24),
    runNonce: clean(body.runNonce, 128)
  };

  if (!payload.levelId || !payload.levelName || !payload.player || !payload.playerId ||
      !payload.status || !payload.modVersion || !payload.runNonce) return null;
  if (!Number.isSafeInteger(payload.timeMs) || payload.timeMs <= 0 || payload.timeMs > 86_400_000) return null;
  return payload;
}

async function submitRun(request: Request, env: Env): Promise<Response> {
  let raw: unknown;
  try {
    raw = await request.json();
  } catch {
    return json({ ok: false, error: "Invalid JSON" }, 400);
  }

  const run = readPayload(raw);
  if (!run) return json({ ok: false, error: "Invalid run payload" }, 400);

  try {
    await env.DB.prepare(`
      INSERT INTO runs (
        run_nonce, level_id, level_name, difficulty, rewards, platformer,
        player, player_id, time_ms, valid, status, reason, safe_mode, mod_version
      ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    `).bind(
      run.runNonce,
      run.levelId,
      run.levelName,
      run.difficulty,
      run.rewards,
      run.platformer ? 1 : 0,
      run.player,
      run.playerId,
      run.timeMs,
      run.valid ? 1 : 0,
      run.valid ? "VALID" : "CHEATED / NOT VALID",
      run.reason ?? "",
      run.safeMode ? 1 : 0,
      run.modVersion
    ).run();
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    if (message.toLowerCase().includes("unique")) {
      return json({ ok: true, duplicate: true }, 200);
    }
    return json({ ok: false, error: "Database write failed" }, 500);
  }

  return json({ ok: true, valid: run.valid }, 201);
}

async function getLeaderboard(url: URL, env: Env): Promise<Response> {
  const requested = Number(url.searchParams.get("limit") ?? "12");
  const limit = Math.max(1, Math.min(Number.isFinite(requested) ? Math.trunc(requested) : 12, 50));
  const levelId = clean(url.searchParams.get("levelId"), 96);
  const includeInvalid = url.searchParams.get("includeInvalid") === "1";

  const bindings: unknown[] = [];
  const combinedWhere = levelId ? "WHERE level_id = ?" : "";
  const rankedWhere = levelId ? "WHERE r.valid = 1 AND r.level_id = ?" : "WHERE r.valid = 1";
  if (levelId) bindings.push(levelId);

  // One best valid result per player and level. Invalid completions are retained
  // and shown as NOT VALID when requested, but can never outrank a valid time.
  const query = includeInvalid ? `
    WITH valid_bests AS (
      SELECT r.* FROM runs r
      JOIN (
        SELECT player_id, level_id, MIN(time_ms) AS best_time
        FROM runs WHERE valid = 1
        GROUP BY player_id, level_id
      ) b ON b.player_id = r.player_id AND b.level_id = r.level_id AND b.best_time = r.time_ms
      WHERE r.valid = 1
      GROUP BY r.player_id, r.level_id
    ), invalid_latest AS (
      SELECT r.* FROM runs r
      JOIN (
        SELECT player_id, level_id, MAX(id) AS newest
        FROM runs WHERE valid = 0
        GROUP BY player_id, level_id
      ) i ON i.newest = r.id
    ), combined AS (
      SELECT * FROM valid_bests
      UNION ALL
      SELECT * FROM invalid_latest
    )
    SELECT level_id AS levelId, level_name AS levelName, difficulty, rewards,
           platformer, player, time_ms AS timeMs, valid, status, reason,
           safe_mode AS safeMode, mod_version AS modVersion, created_at AS createdAt
    FROM combined
    ${combinedWhere}
    ORDER BY valid DESC, CASE WHEN valid = 1 THEN time_ms ELSE 2147483647 END ASC, created_at DESC
    LIMIT ?
  ` : `
    SELECT r.level_id AS levelId, r.level_name AS levelName, r.difficulty, r.rewards,
           r.platformer, r.player, r.time_ms AS timeMs, r.valid, r.status, r.reason, r.safe_mode AS safeMode,
           r.mod_version AS modVersion, r.created_at AS createdAt
    FROM runs r
    JOIN (
      SELECT player_id, level_id, MIN(time_ms) AS best_time
      FROM runs WHERE valid = 1
      GROUP BY player_id, level_id
    ) b ON b.player_id = r.player_id AND b.level_id = r.level_id AND b.best_time = r.time_ms
    ${rankedWhere}
    GROUP BY r.player_id, r.level_id
    ORDER BY r.time_ms ASC, r.created_at ASC
    LIMIT ?
  `;

  bindings.push(limit);
  const result = await env.DB.prepare(query).bind(...bindings).all();
  const runs = (result.results ?? []).map((row) => ({
    ...row,
    valid: Number(row.valid) === 1,
    platformer: Number(row.platformer) === 1,
    safeMode: Number(row.safeMode) === 1
  }));
  return json({ ok: true, runs, refreshedAt: new Date().toISOString() });
}

export default {
  async fetch(request: Request, env: Env): Promise<Response> {
    if (request.method === "OPTIONS") return new Response(null, { status: 204, headers: corsHeaders });
    const url = new URL(request.url);

    if (request.method === "GET" && url.pathname === "/api/v1/health") {
      return json({ ok: true, service: "best-bar-leaderboard", version: "1.5.0" });
    }
    if (request.method === "GET" && url.pathname === "/api/v1/leaderboard") {
      return getLeaderboard(url, env);
    }
    if (request.method === "POST" && url.pathname === "/api/v1/runs") {
      return submitRun(request, env);
    }
    return json({ ok: false, error: "Not found" }, 404);
  }
};
