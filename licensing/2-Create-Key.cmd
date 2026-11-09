@echo off
cd /d "%~dp0.."
where node >nul 2>nul
if errorlevel 1 (
 echo Install Node.js LTS from https://nodejs.org/ first.
 pause
 exit /b 1
)
if exist licensing\.env (
 node --env-file=licensing/.env licensing/issuer.mjs wizard
) else (
 node licensing/issuer.mjs wizard
)
pause
