@echo off
REM ============================================================
REM build.bat — AutoAim v2.0 MVP 构建脚本
REM ============================================================
setlocal enabledelayedexpansion

echo ========================================
echo  AutoAim v2.0 MVP Build Script
echo ========================================
echo.

REM ---- 检测 Qt6 ----
set QT6_DIR=
if exist "C:\Qt\6.8.0\msvc2022_64\lib\cmake\Qt6" (
    set QT6_DIR=C:\Qt\6.8.0\msvc2022_64\lib\cmake\Qt6
) else if exist "C:\Qt\6.7.0\msvc2022_64\lib\cmake\Qt6" (
    set QT6_DIR=C:\Qt\6.7.0\msvc2022_64\lib\cmake\Qt6
) else if exist "C:\Qt\6.5.0\msvc2019_64\lib\cmake\Qt6" (
    set QT6_DIR=C:\Qt\6.5.0\msvc2019_64\lib\cmake\Qt6
)

if not "%QT6_DIR%"=="" (
    echo [OK] Qt6 found: %QT6_DIR%
    echo        Building with UI...
    set CMAKE_QT6=-DCMAKE_PREFIX_PATH="%QT6_DIR%"
) else (
    echo [WARN] Qt6 not found.
    echo        Building only aim_engine.lib (no UI executable).
    echo        To build with UI, set QT6_DIR in this script or install Qt6.
    set CMAKE_QT6=
)

echo.

REM ---- 检测 VS 环境 ----
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] cmake not found. Please run from Visual Studio Developer Command Prompt.
    echo         Or install CMake and add it to PATH.
    pause
    exit /b 1
)

REM ---- 配置 ----
set BUILD_DIR=build_v2.0

echo [1/2] Configuring...
cmake -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=Release %CMAKE_QT6%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configure failed.
    pause
    exit /b 1
)

echo.
echo [2/2] Building...
cmake --build %BUILD_DIR% --config Release -j 8
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Build succeeded!
echo  Output: %BUILD_DIR%\Release\
echo ========================================

endlocal
