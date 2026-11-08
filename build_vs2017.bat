@echo off
rem Builds a single static 32-bit exe (runs on Windows 7 32-bit AND 64-bit).
rem Needs: Visual Studio 2017 with "Windows XP support for C++" (v141_xp), and CMake in PATH.
rem Run from a "Developer Command Prompt for VS 2017".
if not exist build mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A Win32 -T v141_xp .. || goto :err
cmake --build . --config Release || goto :err
echo.
echo Done: build\Release\DiscImageStudio.exe
goto :eof
:err
echo BUILD FAILED
exit /b 1
