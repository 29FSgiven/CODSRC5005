@echo off

echo.
echo Compiling...
cd bin

g++.exe -g -c -O0 -Wall "../src/main.cpp" -D_DEBUG
g++.exe -g -c -O0 -Wall "../src/checksumengine.cpp" -D_DEBUG
g++.exe -g -c -O0 -Wall "../src/symbollist.includer.c" -D_DEBUG
g++.exe -g -c -O0 -Wall "../src/pehlp.cpp" -D_DEBUG
g++.exe -g -c -O0 -Wall "../src/elfhlp.cpp" -D_DEBUG
gcc.exe -g -std=c99 -c -O0 -Wall "../src/adler32.c" -D_DEBUG
gcc.exe -g -std=c99 -c -O0 -Wall "../src/rc5_32.c" -D_DEBUG
gcc.exe -g -std=c99 -c -O0 -Wall "../src/blake/blake2s-ref.c" -D_DEBUG
g++.exe -g -c -O0 -Wall "../src/common/tiny_msg.cpp" -D_DEBUG
g++.exe -g -c -O0 -Wall "../src/common/basic.cpp" -D_DEBUG
g++.exe -g -c -O0 -Wall "../src/common/rsa_verify.cpp" -D_DEBUG
echo Linking...

g++.exe -shared -static -L. -specs=../linker-specs.ld -o xac_client.lib rsa_verify.o rc5_32.o pehlp.o checksumengine.o symbollist.includer.o main.o adler32.o tiny_msg.o elfhlp.o basic.o blake2s-ref.o -ltomcrypt -lstdc++ -lkernel32 -luser32 -lpsapi -Wl,-Map=iam_clint.map

..\..\exeobfus.exe xac_client.lib

move xac_client.lib %LOCALAPPDATA%\CallofDuty4MW\bin\cod4x_018\iam_clint.dll

echo Cleaning Up...
del *.o
pause
exit