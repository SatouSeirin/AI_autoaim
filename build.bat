@echo off
chcp 65001 >nul
title MiniSense Build

echo ========== MiniSense ???? ==========
echo.

:: 1. ????
echo [1/4] ?????...
taskkill /f /im auto_aim_ui.exe >nul 2>&1
del /f build\Release\auto_aim_ui.exe >nul 2>&1

:: 2. VS ??
echo [2/4] ?? VS 2026 ??...
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
if %errorlevel% neq 0 (
    echo ??: ??? VS 2026!
    pause
    exit /b 1
)

:: 3. CMake ??
echo [3/4] ?? CMake...
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.8.2/6.8.2/msvc2022_64 >nul 2>&1
if %errorlevel% neq 0 (
    echo ????????? build ??...
    rmdir /s /q build >nul 2>&1
    cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.8.2/6.8.2/msvc2022_64
    if %errorlevel% neq 0 (
        echo CMake ???????? Qt ??
        pause
        exit /b 1
    )
)

:: 4. ??
echo [4/4] ???...
cmake --build build --config Release
set BUILD_RESULT=%errorlevel%
echo.
if %BUILD_RESULT% equ 0 (
    echo ======== ?????========
    echo ??: %cd%\build\Release\auto_aim_ui.exe
    echo.
    echo ????: build\Release\auto_aim_ui.exe
) else (
    echo ======== ???? ========
)
echo.
pause
