@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not exist .env (
 echo Mo 3-Setup-Bot.cmd truoc.
 pause
 exit /b 1
)
where node >nul 2>nul
if errorlevel 1 goto failed
node -e "if(Number(process.versions.node.split('.')[0])<24)process.exit(1)"
if errorlevel 1 goto failed
if not exist node_modules\discord.js\package.json (
 call npm ci --omit=dev
 if errorlevel 1 goto failed
)
node --env-file=.env discord-bot.mjs
pause
exit /b
:failed
echo Can Node.js 24 LTS va mang de cai thu vien. Xem thong bao phia tren.
pause
