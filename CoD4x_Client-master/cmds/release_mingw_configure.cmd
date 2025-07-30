@echo off

mkdir build_mingw_release
cd build_mingw_release
cmake -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cd ..
