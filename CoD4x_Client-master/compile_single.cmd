@echo off
@set binfilename=cod4x_017

set bindir=%LOCALAPPDATA%\CallofDuty4MW
if not exist %bindir% (
    mkdir %bindir%
)

set bindir=%bindir%\bin
if not exist %bindir% (
    mkdir %bindir%
)

set bindir=%bindir%\%binfilename%
if not exist %bindir% (
    mkdir %bindir%
)


echo.
echo Compiling...

cd bin


gcc.exe -c %FLAGS% "../src/xassets.c"

echo Compiling NASM...
::echo %CD%
::pause
nasm -f win32 ../src/callbacks.asm --prefix _ -ocallbacks.o
nasm -f win32 ../src/client_callbacks.asm --prefix _ -o client_callbacks.o
nasm -f win32 ../src/mss32jumptable.asm --prefix _ -o mss32jumptable.o
nasm -f win32 ../src/fsdword.asm --prefix _ -o fsdword.o

cd "../"
echo Linking...
gcc.exe -shared %FLAGS% -o bin/%binfilename%.dll bin/*.o sbin/*.o -static-libgcc -static -lstdc++ -Llib/ -ludis86 -ltomcrypt -lseh -lmbedtls_win32 -lole32 -loleaut32 -luuid -lwsock32 -lws2_32 -lwinmm -luser32 -lgdi32 -lkernel32 -lcrypt32 -ld3dx9_34 -lpsapi -Wl,-Map=%bindir%\%binfilename%.map,-fPic,--stack,8388608

echo Cleaning Up...
cd bin
del "*.o"


move %binfilename%.dll %bindir%\%binfilename%.dll

cd ..