@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not exist .env (
 echo Can .env da setup. Khong tao lai issuer.
 pause
 exit /b 1
)
node --env-file=.env connect-website.mjs
if errorlevel 1 goto finished
node --env-file=.env register-commands.mjs
if not errorlevel 1 echo Da dang ky /buy cho khach trong server Discord.
:finished
pause
