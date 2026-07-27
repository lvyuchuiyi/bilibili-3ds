@echo off
chcp 65001 >nul
title ??????

if "%1"=="" (
  echo ????: setdesc.cmd YOUR_GITHUB_TOKEN
  pause
  exit /b
)

set CURL=C:\Windows\System32\curl.exe

echo ????????...
%CURL% -s -X PATCH ^
  -H "Authorization: token %1" ^
  -H "Content-Type: application/json" ^
  -d "{\"name\":\"bilibili-3ds\",\"description\":\"Nintendo 3DS BiliBili video client / 3DS ?? B ????\"}" ^
  https://api.github.com/repos/lvyuchuiyi/bilibili-3ds

echo.
echo ????????????
pause
