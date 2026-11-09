@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not exist .env (
 echo Mo 3-Setup-Bot.cmd truoc.
 pause
 exit /b 1
)
node --env-file=.env payment-server.mjs
pause
