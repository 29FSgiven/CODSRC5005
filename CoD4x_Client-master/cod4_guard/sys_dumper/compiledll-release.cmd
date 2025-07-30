@echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\Program Files (x86)\mingw-w64\i686-5.3.0-win32-dwarf-rt_v4-rev0\mingw32\bin;%path%


echo.
echo Compiling...

gcc.exe -g -std=c99 -c -O0 -Wall "main.c"
gcc.exe -g -std=c99 -c -O0 -Wall "rc5_32.c"
gcc.exe -g -std=c99 -c -O0 -Wall "cleanup_lib.c"
gcc.exe -g -std=c99 -c -O0 -Wall "blake/blake2s-ref.c"

echo Linking...

gcc.exe -g -shared -o ac_dumper.dll main.o rc5_32.o blake2s-ref.o cleanup_lib.o -lpsapi
echo Cleaning Up...
del *.o
pause
exit