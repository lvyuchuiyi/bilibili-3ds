@echo off
chcp 65001 >nul
title ?? GitHub Release

if "%1"=="" (
  echo ????: release.cmd YOUR_GITHUB_TOKEN
  echo.
  echo 1. ?? https://github.com/settings/tokens
  echo 2. ????? repo ??? token
  echo 3. ??: release.cmd ghp_xxxxxxxxxxxx
  pause
  exit /b
)

set CURL=C:\Windows\System32\curl.exe
set TOKEN=%1

echo ???? Release v1.0.0 ...
%CURL% -s -X POST ^
  -H "Authorization: token %TOKEN%" ^
  -H "Content-Type: application/json" ^
  -d "{\"tag_name\":\"v1.0.0\",\"name\":\"BiliBili 3DS v1.0.0\",\"body\":\"???????\",\"draft\":false,\"prerelease\":false}" ^
  https://api.github.com/repos/lvyuchuiyi/bilibili-3ds/releases

echo.
echo ??????????????? .cia ???
echo https://github.com/lvyuchuiyi/bilibili-3ds/releases
pause
