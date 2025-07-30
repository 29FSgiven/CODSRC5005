@echo off

echo.
echo Compiling...
cd bin
g++.exe -s -c -O3 -Wall "../src/main.cpp"
g++.exe -s -c -O3 -Wall "../src/checksumengine.cpp"
g++.exe -s -c -O3 -Wall "../src/symbollist.includer.c"
g++.exe -s -c -O3 -Wall "../src/pehlp.cpp"
g++.exe -s -c -O3 -Wall "../src/elfhlp.cpp"
gcc.exe -s -std=c99 -c -O3 -Wall "../src/adler32.c"
gcc.exe -s -std=c99 -c -O3 -Wall "../src/rc5_32.c"
gcc.exe -s -std=c99 -c -O3 -Wall "../src/blake/blake2s-ref.c"
g++.exe -s -c -O3 -Wall "../src/common/tiny_msg.cpp"
g++.exe -s -c -O3 -Wall "../src/common/basic.cpp"
g++.exe -s -c -O3 -Wall "../src/common/rsa_verify.cpp"
echo Linking...

g++.exe -s -shared -static -L. -specs=..\linker-specs.ld -o xac_client.lib rsa_verify.o rc5_32.o pehlp.o checksumengine.o symbollist.includer.o main.o adler32.o tiny_msg.o elfhlp.o basic.o blake2s-ref.o -ltomcrypt -lstdc++ -lkernel32 -luser32 -lpsapi -Wl,-Map=iam_clint.map

..\..\exeobfus.exe xac_client.lib

REM xac_client.lib %LOCALAPPDATA%\CallofDuty4MW\bin\cod4x_018\iam_clint.dll

echo Cleaning Up...
del *.o
pause
exit