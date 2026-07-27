@echo off
chcp 65001 >nul
title BiliBili 3DS - 安装与编译

echo ============================================
echo   BiliBili 3DS - 安装与编译脚本
echo ============================================
echo.

REM Check if devkitPro is already installed
if exist "C:\devkitPro" (
    echo [OK] devkitPro 已安装
    goto :compile
)

echo [..] 正在下载 devkitPro 安装器...
where curl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERR] 未找到 curl，请手动下载安装器
    echo       https://github.com/devkitPro/installer/releases
    pause
    exit /b 1
)

curl.exe -L -o "%TEMP%\devkitProUpdater.exe" ^
    "https://github.com/devkitPro/installer/releases/download/v3.0.3/devkitProUpdater-3.0.3.exe"
if %errorlevel% neq 0 (
    echo [ERR] 下载失败，请手动下载安装器
    echo       https://github.com/devkitPro/installer/releases
    pause
    exit /b 1
)
echo [OK] 下载完成
echo.
echo ============================================
echo  请在弹出的安装窗口中:
echo  1. 保持默认安装路径 C:\devkitPro
echo  2. 在组件选择中勾选 "3DS Development"
echo  3. 等待安装完成后再关闭此窗口
echo ============================================
echo.
start /wait "" "%TEMP%\devkitProUpdater.exe"

if not exist "C:\devkitPro" (
    echo [ERR] devkitPro 未安装成功
    echo       请手动下载: https://github.com/devkitPro/installer/releases
    pause
    exit /b 1
)
echo [OK] devkitPro 安装成功

:compile
echo.
echo [..] 正在编译 BiliBili 3DS ...

REM Set up environment for devkitARM
set DEVKITPRO=C:\devkitPro
set DEVKITARM=%DEVKITPRO%\devkitARM

if not exist "%DEVKITARM%" (
    echo [ERR] 未找到 devkitARM，请确认安装了 3DS Development 组件
    pause
    exit /b 1
)

REM Compile both .3dsx and .cia
make release -j%NUMBER_OF_PROCESSORS% 2>&1

if %errorlevel% neq 0 (
    echo [ERR] 编译失败，请检查上面的错误信息
    pause
    exit /b 1
)

echo [OK] 编译成功!
echo.
echo ============================================
echo   输出文件:
echo     bilibili3ds.3dsx  - Homebrew Launcher
echo     bilibili3ds.cia   - FBI 安装（直接上主屏幕）
echo     bilibili3ds.elf   - 调试 ELF
echo     bilibili3ds.smdh  - 图标元数据
echo.
echo   安装方式:
echo     1. CIA: 拷贝到SD卡，用 FBI 安装
echo     2. 3DSX: 拷贝到 /3ds/bilibili3ds/ 目录
echo ============================================
echo.
mkdir outputs 2>nul
copy /Y bilibili3ds.3dsx outputs\ 2>nul
copy /Y bilibili3ds.cia outputs\ 2>nul 2>nul
copy /Y bilibili3ds.smdh outputs\ 2>nul
echo [OK] 文件已复制到 outputs 目录
echo.
pause