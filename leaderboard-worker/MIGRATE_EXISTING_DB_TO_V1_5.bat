@echo off
setlocal
cd /d "%~dp0"
where npm >nul 2>nul || (echo ERROR: Install Node.js first.& pause& exit /b 1)
if not exist wrangler.toml (
  echo ERROR: Copy wrangler.toml.example to wrangler.toml and set your database ID first.
  pause
  exit /b 1
)
call npm install || goto fail
call npx wrangler d1 execute best-bar-leaderboard --remote --file=migration_v1_5.sql || goto fail
echo.
echo Existing database upgraded to v1.5 metadata columns.
pause
exit /b 0
:fail
echo.
echo Migration failed. Do not run it twice; read the error above.
pause
exit /b 1
