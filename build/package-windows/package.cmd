@echo off

rem
rem **** Run this from the top of the debugger source tree: ****
rem
rem Usage: package.cmd DEBUGGER_PLATFORM DEBUGGER_CONFIGURATION DEBUGGER_VERSION
rem
rem **** DEBUGGER_PLATFORM is { Win32, x64 } ****
rem **** DEBUGGER_CONFIGURATION is { Release, Debug } ****
rem **** DEBUGGER_VERSION is a version string; e.g. 0.7.0  ****

if "%3" == "" goto usage
if "%4" NEQ "" goto usage

setlocal

set DEBUGGER_PLATFORM=%1
echo DEBUGGER_PLATFORM is %DEBUGGER_PLATFORM%

set DEBUGGER_CONFIGURATION=%2
echo DEBUGGER_CONFIGURATION is %DEBUGGER_CONFIGURATION%

set DEBUGGER_VERSION=%3
echo DEBUGGER_VERSION is %DEBUGGER_VERSION%

set DEBUGGER_PACKAGE_WINDOWS_PATH=.\build\package-windows
set PYTHONPATH=%PYTHONPATH%;.\build

python %DEBUGGER_PACKAGE_WINDOWS_PATH%\packagezip.py %DEBUGGER_PLATFORM% %DEBUGGER_CONFIGURATION% %DEBUGGER_VERSION%

endlocal
goto end

:usage
echo Usage: package.cmd platform configuration version
:end
