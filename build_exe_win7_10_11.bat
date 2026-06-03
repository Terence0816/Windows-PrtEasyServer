@echo off
setlocal EnableDelayedExpansion

cd /d "%~dp0"

set "PY_CMD="
set "LOCAL_PY_DIR=%CD%\Python38"
set "LOCAL_PY_EXE=%LOCAL_PY_DIR%\python.exe"
set "LOCAL_PIP_EXE=%LOCAL_PY_DIR%\Scripts\pip.exe"
set "LOCAL_PY_ZIP=%CD%\Python38.zip"
set "PYTHON38_ZIP_URL=https://github.com/Terence0816/Windows-PrtEasyServer/releases/download/v1.0.0.0/Python38.zip"

if not exist "%LOCAL_PY_EXE%" (
    if not exist "%LOCAL_PY_ZIP%" (
        echo Local Python38.zip was not found.
        echo Downloading Python38.zip from the compatibility release...
        powershell -NoProfile -ExecutionPolicy Bypass -Command ^
            "$url='%PYTHON38_ZIP_URL%'; " ^
            "$dest=Join-Path (Get-Location) 'Python38.zip'; " ^
            "try { [Net.ServicePointManager]::SecurityProtocol = [enum]::ToObject([Net.SecurityProtocolType], 3072) } catch {}; " ^
            "$wc=New-Object System.Net.WebClient; " ^
            "$wc.DownloadFile($url, $dest)"
        if errorlevel 1 (
            echo.
            echo Failed to download Python38.zip from the compatibility release.
            pause
            exit /b 1
        )
    )
    if exist "%LOCAL_PY_ZIP%" (
        echo Local Python 3.8 folder was not found.
        echo Extracting Python38.zip to "%LOCAL_PY_DIR%"...
        powershell -NoProfile -ExecutionPolicy Bypass -Command ^
            "$zip=(Resolve-Path '%LOCAL_PY_ZIP%').Path; " ^
            "$dest=Join-Path (Get-Location) 'Python38'; " ^
            "Add-Type -AssemblyName System.IO.Compression.FileSystem; " ^
            "if (Test-Path $dest) { Remove-Item -LiteralPath $dest -Recurse -Force }; " ^
            "New-Item -ItemType Directory -Path $dest -Force | Out-Null; " ^
            "[System.IO.Compression.ZipFile]::ExtractToDirectory($zip, $dest)"
        if errorlevel 1 (
            echo.
            echo Failed to extract Python38.zip.
            pause
            exit /b 1
        )
    )
)

if exist "%LOCAL_PY_EXE%" (
    set "PY_CMD="%LOCAL_PY_EXE%""
)

if defined PY_CMD (
    if not exist "%LOCAL_PIP_EXE%" (
        echo Bootstrapping pip for local Python 3.8...
        "%LOCAL_PY_EXE%" -m ensurepip --upgrade
        if errorlevel 1 (
            echo.
            echo Failed to initialize pip in local Python38.
            pause
            exit /b 1
        )
    )
)

if not defined PY_CMD if defined PY38_EXE (
    if exist "%PY38_EXE%" (
        set "PY_CMD="%PY38_EXE%""
    )
)

where py >nul 2>&1
if not defined PY_CMD if not errorlevel 1 (
    py -3.8 --version >nul 2>&1
    if not errorlevel 1 (
        set "PY_CMD=py -3.8"
    )
)

if not defined PY_CMD (
    where python >nul 2>&1
    if not errorlevel 1 (
        for /f "tokens=2" %%V in ('python --version 2^>^&1') do set "PY_VER=%%V"
        if /i "!PY_VER:~0,4!"=="3.8." (
            set "PY_CMD=python"
        )
    )
)

if not defined PY_CMD (
    if exist "%LocalAppData%\Programs\Python\Python38\python.exe" (
        set "PY_CMD="%LocalAppData%\Programs\Python\Python38\python.exe""
    )
)

if not defined PY_CMD (
    if exist "C:\Python38\python.exe" (
        set "PY_CMD="C:\Python38\python.exe""
    )
)

if not defined PY_CMD (
    echo Python 3.8 was not found.
    echo The script can auto-download Python38.zip from the compatibility release when needed.
    echo You can also place Python38.zip next to this BAT file, or provide a Python38 folder.
    echo You can also set PY38_EXE to the full python.exe path.
    echo Please install Python 3.8.x, then try again.
    pause
    exit /b 1
)

echo Starting Windows 7 build...
echo Using %PY_CMD%
%PY_CMD% build.py --target win7
if errorlevel 1 (
    echo.
    echo Win7 build failed.
    pause
    exit /b 1
)

echo.
echo Win7 build completed.
echo Version: v1.2.0.0
echo Output: dist\PrtEasyServer_win7.exe
pause
exit /b 0
