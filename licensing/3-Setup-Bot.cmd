@echo off
chcp 65001 >nul
cd /d "%~dp0"
where node >nul 2>nul
if errorlevel 1 (
 echo Cai Node.js 24 LTS tu https://nodejs.org/ roi mo lai file nay.
 pause
 exit /b 1
)
node setup-bot.mjs
pause
