@echo off
setlocal
cd /d "%~dp0"
where npm >nul 2>nul || (echo ERROR: Install Node.js first.& pause& exit /b 1)
if not exist wrangler.toml (
  copy /Y wrangler.toml.example wrangler.toml >nul
  echo.
  echo Created wrangler.toml. Run: npx wrangler d1 create best-bar-leaderboard
  echo Then paste the database_id into wrangler.toml and run this file again.
  pause
  exit /b 0
)
call npm install || goto fail
call npx wrangler d1 execute best-bar-leaderboard --remote --file=schema.sql || goto fail
call npx wrangler deploy || goto fail
echo.
echo Deployment complete. Copy the Worker URL into Best Bar settings.
pause
exit /b 0
:fail
echo.
echo Deployment failed. Read the error above.
pause
exit /b 1
