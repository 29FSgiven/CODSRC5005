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

SET FLAGS=-g -Wall -O0 -D COD4XDEV -D NOD3DVALIDATION -D NOANTIREVERSING
SET FLAGS_O3=-g -Wall -O0 -D COD4XDEV -D NOD3DVALIDATION -D NOANTIREVERSING

call compiledll_common.cmd


if exist bin/exeobfus.exe (

) else (
	echo Compiling exeobfus
	cd bin
	del *.o 2> NUL
	gcc.exe -g -std=c99 -c -O0 -Wall "../src/exeobfus/exeobfus.c"
	gcc.exe -o exeobfus.exe *.o -static-libgcc -static -lstdc++
	echo exeobfus Compiled!
	cd ../
)

pause
::exit
