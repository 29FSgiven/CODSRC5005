@echo off

mkdir build_mingw_debug
cd build_mingw_debug
cmake -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
cd ..
