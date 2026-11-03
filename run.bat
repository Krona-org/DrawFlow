@echo off
set "PATH=C:\Qt\6.10.2\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%"
if exist "%~dp0build-release\dcseditor_qt6.exe" (
    start "" "%~dp0build-release\dcseditor_qt6.exe"
) else (
    start "" "%~dp0build\dcseditor_qt6.exe"
)
