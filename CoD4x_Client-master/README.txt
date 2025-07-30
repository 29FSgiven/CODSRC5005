You need Mingw in version:
Thread model: win32 
gcc version 8.1.0 (i686-win32-dwarf-rev0, Built by MinGW-W64 project)

Using other version could result in linker errors.

Instructions to build client the first time:

1. enter ./src/discord-rpc-api & run compilediscordrpc.cmd
2. enter ./src/diskinfo & run compiledll-jpg.cmd & run compiledll-release.cmd
3. enter ./src/libudis86 & run compilelibudis86.cmd
4. enter ./src/exeobfus & run compiledll-release.cmd
5. enter ./src/libseh-0.0.4 & run mingw32-make & copy ./src/libseh-0.0.4/build/libseh.a to ./lib
6. enter ./src/mbedtls & run build.cmd
7. enter ./src/steam_api & run compilesteamapi.cmd
8. enter ./src/tomcrypt & run compile.cmd
9. enter ./freetype-2.9/ & run mingw32-make & copy ./objs/freetype.a to ../lib/libfreetype.a
10. enter ./cod4_guard/sys_dumper & run compiledll-release.cmd & copy ac_dumper.dll to CoD4-root directory
11. copy C:\Program Files (x86)\mingw-w64\i686-8.1.0-win32-dwarf-rt_v6-rev0\mingw32\i686-w64-mingw32\lib\dllcrt2.o to ./mingwcrt

Finally you can run ./compiledll-release.cmd

Build/deploy using CMake build system.

build_mingw.cmd, clean_mingw.cmd and rebuild_mingw.cmd are included for appropriate actions. All files designer to be used in VSCode as command for tasks.

CMake build system uses few environment variables to automatically deploy binaries after build (values are for example):

COD4XCLIENT_CMAKE_BUILD_VERSION 018
COD4XCLIENT_CMAKE_GAME_ROOT D:\Games\CoD4-1.8_dev