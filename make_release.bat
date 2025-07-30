
REM make_release.bat

@echo off
REM --------------------------------------------------------
REM Usage: make_release.bat <version>
REM    e.g. make_release.bat 0.9.16
REM --------------------------------------------------------

IF "%~1"=="" (
    echo Usage: %~n0 ^<version^>
    exit /b 1
)

set "VERSION=%~1"
set "RELEASE_NAME=mxbmrp2-v%VERSION%"

echo.
echo === Building release %RELEASE_NAME% ===

REM 1) Create folder structure
mkdir ".\Releases\%RELEASE_NAME%\mxbmrp2_data" 2>nul

REM 2) Copy data directory recursively
xcopy ".\mxbmrp2_data"    ".\Releases\%RELEASE_NAME%\mxbmrp2_data" /E /I /Y

REM 3) Copy & rename the DLL → .dlo
copy  ".\x64\Release\mxbmrp2.dll"  ".\Releases\%RELEASE_NAME%\mxbmrp2.dlo"

REM 4) Copy docs
copy  ".\README.md"   ".\Releases\%RELEASE_NAME%\" 
copy  ".\LICENSE.txt" ".\Releases\%RELEASE_NAME%\" 

REM 5) Zip it up
"C:\Program Files\7-Zip\7z.exe" a ".\Releases\%RELEASE_NAME%.zip" ".\Releases\%RELEASE_NAME%\*"

echo.
echo ZIP complete: Releases\%RELEASE_NAME%.zip

REM 6) Build NSIS installer

makensis -DPLUGIN_VERSION="%VERSION%" mxbmrp2.nsi

if %ERRORLEVEL% neq 0 (
  echo [ERROR] NSIS build failed!
  exit /b %ERRORLEVEL%
)

echo.
echo Installer built: mxbmrp2-v%VERSION%-Setup.exe
pause
