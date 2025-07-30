@echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\Program Files (x86)\mingw-w64\i686-5.3.0-win32-dwarf-rt_v4-rev0\mingw32\bin;%path%


echo.
echo Compiling...

g++.exe -g -fno-strict-aliasing -c -O0 -Wall "compressor.cpp" -DFASTCOMPILE

echo Linking...
echo Cleaning Up...

move compressor.o ../../sbin

pause
exit
