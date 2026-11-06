@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0GameData.ps1" -Mode Backup
pause
