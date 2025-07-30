@echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\Program Files (x86)\mingw-w64\i686-5.3.0-win32-dwarf-rt_v4-rev0\mingw32\bin;%path%


echo.
echo Compiling...

g++.exe -s -fno-strict-aliasing -c -O3 -Wall "compressor.cpp"

echo Linking...
echo Cleaning Up...

move compressor.o ../../sbin

pause
exit
