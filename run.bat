@echo off
REM Change directory to 'code'
cd code

REM Run the build script inside 'code'
call build.bat

REM Go back to parent directory
cd ..

REM Run the executable
build\win32_platform.exe
