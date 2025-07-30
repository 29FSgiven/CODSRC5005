rem @echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\Program Files (x86)\mingw-w64\i686-5.3.0-win32-dwarf-rt_v4-rev0\mingw32\bin;%path%


echo.
echo Compiling...

gcc.exe -s -fno-strict-aliasing -std=c99 -c -O3 -Wall "system_patch.c"

echo Linking...
echo Cleaning Up...

move system_patch.o ../../sbin

pause
exit