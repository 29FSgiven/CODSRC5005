@set path=C:\MinGW\bin;%path%
@echo off

echo Compiling C-code...
cd bin
g++ -std=c++11 -m32 -Wall -O0 -g -fno-omit-frame-pointer -march=nocona -D WINVER=0x501 -c ..\src\*.cpp -I..\src\tomcrypt
g++ -std=c++11 -m32 -Wall -O0 -g -fno-omit-frame-pointer -march=nocona -D WINVER=0x501 -c ..\src\win32\sys_win32.cpp

cd ..\

echo Linking...
gcc -m32 -g -Wl,--nxcompat,--image-base,0x8040000,--stack,0x800000 -o bin\filesync.exe bin\*.o -Llib -lm -lws2_32 -lwsock32 -lgdi32 -lcrypt32 -lwinmm -ltomcrypt -static-libgcc -static -lstdc++ 

echo Cleaning up...
cd bin
del *.o

echo Done!
pause

