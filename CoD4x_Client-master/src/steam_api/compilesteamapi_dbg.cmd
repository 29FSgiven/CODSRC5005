@echo off
@set path=%LOCALAPPDATA%\nasm;%path%

echo.
echo Compiling...

g++.exe -c -g -O0 -Wall "steam_api.cpp"
g++.exe -c -g -O0 -Wall "steam_interface.cpp"

move steam_interface.o ..\..\sbin
move steam_api.o ..\..\sbin
pause