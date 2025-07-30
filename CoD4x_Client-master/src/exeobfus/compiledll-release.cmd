@echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\MinGW\bin;%path%

echo.
echo Compiling...

gcc.exe -g -std=c99 -c -O0 -Wall "exeobfus.c"

gcc.exe -o exeobfus.exe *.o -static-libgcc -static -lstdc++ -L../../lib/ -ludis86

echo Cleaning Up...
del *.o

move exeobfus.exe ..\..\

pause
exit
