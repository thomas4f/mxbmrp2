@echo off

REM Define the release name
set "release=mxbmrp2-v0.9.4"

REM Create the release directories
mkdir ".\Releases\%release%\mxbmrp2_data" 2>nul

REM Copy the mxbmrp2_data directory recursively
xcopy ".\mxbmrp2_data" ".\Releases\%release%\mxbmrp2_data" /E /I /Y

REM Copy and rename the DLL to DLO
copy ".\x64\Release\mxbmrp2.dll" ".\Releases\%release%\mxbmrp2.dlo"

REM Copy README and LICENSE
copy ".\README.md" ".\Releases\%release%\"
copy ".\LICENSE.txt" ".\Releases\%release%\"

REM Use 7z to compress the release directory
"C:\Program Files\7-Zip\7z.exe" a ".\Releases\%release%.zip" ".\Releases\%release%\*"

echo Release package created: .\Releases\%release%.zip
pause
