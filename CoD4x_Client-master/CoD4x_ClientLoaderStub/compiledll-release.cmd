@echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\MinGW\bin;%path%

echo.
echo Compiling...
SET FLAGS=-s -Os

cd bin
gcc.exe -c %FLAGS% -Wall "../src/cmd.c"
gcc.exe -c %FLAGS% -Wall "../src/qcommon_parsecmdline.c"
gcc.exe -c %FLAGS% -Wall "../src/win_main.c"
gcc.exe -c %FLAGS% -Wall "../src/sys_patch.c"
gcc.exe -c %FLAGS% -Wall "../src/qshared.c"
gcc.exe -c %FLAGS% -Wall "../src/sec_sign.c"
gcc.exe -c %FLAGS% -Wall "../src/sec_rsa_functions.c"
gcc.exe -c %FLAGS% -Wall "../src/sec_common.c"
gcc.exe -c %FLAGS% -Wall "../src/sec_crypto.c"
gcc.exe -c %FLAGS% -Wall "../src/sec_init.c"


gcc.exe -c %FLAGS% -Wall "../src/zlib/adler32.c"

echo Compiling NASM...
::echo %CD%
::pause
nasm -f win32 ../src/mss32jumptable.asm --prefix _ -o mss32jumptable.o
nasm -f win32 ../src/callbacks.asm --prefix _ -ocallbacks.o

cd "../"
echo Linking...
REM gcc.exe -shared -s -o bin/mss32.dll bin/*.o sbin/*.o -static-libgcc -static -lstdc++  -Llib/ -ludis86 -ltomcrypt -lmbedtls_win32 -lole32 -loleaut32 -luuid -lwsock32 -lws2_32 -lwinmm -luser32 -lgdi32 -lkernel32 -ld3dx9_34 -lpsapi -Wl,-fPic,--stack,8388608
gcc.exe -shared -s -o bin/mss32.dll bin/*.o -static-libgcc -lkernel32 -Llib/ -ltomcrypt -Wl,--exclude-libs,msvcrt.a,-fPic,--stack,8388608

echo Cleaning Up...
cd bin
del "*.o"

pause
exit
