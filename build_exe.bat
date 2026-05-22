@echo off
setlocal

cd /d "%~dp0"

set "PY_CMD="
where python >nul 2>&1
if not errorlevel 1 (
    python --version >nul 2>&1
    if not errorlevel 1 (
        set "PY_CMD=python"
    )
)
if not defined PY_CMD (
    where py >nul 2>&1
    if not errorlevel 1 (
        py --version >nul 2>&1
        if not errorlevel 1 (
            set "PY_CMD=py"
        )
    )
)
if not defined PY_CMD (
    echo Python was not found.
    echo Please install Python 3, then try again.
    pause
    exit /b 1
)

echo Starting build...
echo Using %PY_CMD%
%PY_CMD% build.py
if errorlevel 1 (
    echo.
    echo Build failed.
    pause
    exit /b 1
)

echo.
echo Build completed.
echo Version: v1.1.1.0
echo Output: dist\PrtEasyServer.exe
pause
exit /b 0
