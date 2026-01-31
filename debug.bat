@echo off
call config.bat

setlocal

REM Set up the Visual Studio envionment (for cl command)
call "%VCVARS_PATH%" x64

devenv "%DEBUG_EXE_NAME%