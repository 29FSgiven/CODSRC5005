@echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\Program Files (x86)\mingw-w64\i686-5.3.0-win32-dwarf-rt_v4-rev0\mingw32\bin;%path%


echo.
echo Compiling...

gcc.exe -g -std=c99 -c -O0 -Wall "main.c"
gcc.exe -g -std=c99 -c -O0 -Wall "pehlp.c"
gcc.exe -g -std=c99 -c -O0 -Wall "adler32.c"

echo Linking...

gcc.exe -g -o iam_clint.exe main.o pehlp.o adler32.o -lpsapi
echo Cleaning Up...
del *.o
pause
exit