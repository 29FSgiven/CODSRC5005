@echo off

cls
color 40
echo -------------------------------------------------------------
echo -                          WARNING                          -
echo -         This build script is for DEVELOPMENT ONLY!        -
echo -   DO NOT GIVE THE COMPILED DLL TO UNAUTHORIZED PERSONS!   -
echo -     If you meant to run the release script, exit now!     -
echo -------------------------------------------------------------
pause
color 07
cls

::if not exist lib\libsteam_api.a (
	::echo Compiling Steam API
	::cd sbin
	::g++.exe -c -s -O0 -Wall "../src/steam_api/steam_interface.cpp"
	::echo Steam API Compiled!
	::cd ../
::)

::just use goto here later
if not exist sbin\steam_interface.o (
	echo Compiling Steam API
	cd sbin
	g++.exe -c -s -O0 -Wall "../src/steam_api/steam_interface.cpp"
	echo Steam API Compiled!
	cd ../
)

if not exist sbin\system_patch.o (
	echo Compiling Disk Info
	cd sbin
	gcc.exe -s -fno-strict-aliasing -std=c99 -c -O3 -Wall "../src/diskinfo/system_patch.c"
	cd ../
	echo Disk Info Compiled!
)

if not exist lib\libtomcrypt.a (
	echo Compiling TomCrypt Library
	cd src\tomcrypt
	call compileauto.cmd
	echo TomCrypt Compiled!
	cd ../../
)


g++.exe -shared %FLAGS% -o bin/%binfilename%.dll bin/*.o sbin/*.o -static-libgcc -static -lstdc++ -Llib/ -ludis86 -ltomcrypt -lversion -lseh -lmbedtls_win32 -ldiscord_rpc -lole32 -loleaut32 -luuid -lwsock32 -lws2_32 -lwinmm -luser32 -lgdi32 -lkernel32 -lcrypt32 -lpsapi -ld3dx9_34 -Wl,-Map=%bindir%\%binfilename%.map,-fPic,--stack,8388608

move %binfilename%.dll %bindir%\%binfilename%.dll

pause
::exit
