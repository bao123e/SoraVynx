@echo off
cd /d "%~dp0"
if exist cloudflared.exe (
 cloudflared.exe tunnel --url http://127.0.0.1:8787
) else (
 echo Tai cloudflared-windows-amd64.exe tu link chinh thuc:
 echo https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe
 echo Doi ten thanh cloudflared.exe, dat cung thu muc voi file nay roi mo lai.
)
pause
