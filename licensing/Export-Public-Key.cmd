@echo off
chcp 65001 >nul
cd /d "%~dp0"
where node >nul 2>nul
if errorlevel 1 (
 echo Can Node.js LTS tu https://nodejs.org/
 pause
 exit /b 1
)
node Export-Public-Key.mjs
pause
