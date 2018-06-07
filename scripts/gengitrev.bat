@echo off
setlocal enableextensions
setlocal enabledelayedexpansion
for /f %%t in ('git describe --tags --dirty') do set PAL_GIT_REVISION=%%t
if "!PAL_GIT_REVISION!" == "" (
	for %%t in (%~dp0..\.gitignore) do set PAL_GIT_REVISION=%%~tt
)
findstr /c:"#define PAL_GIT_REVISION \"!PAL_GIT_REVISION!\"" "%~dp0..\generated.h" >nul 2>&1
if errorlevel 1 (
    echo Generating Git revision header: !PAL_GIT_REVISION! 1>&2
    echo #define PAL_GIT_REVISION "!PAL_GIT_REVISION!" > "%~dp0..\generated.h"
)
exit /b 0
