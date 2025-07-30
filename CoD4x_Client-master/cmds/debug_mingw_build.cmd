@echo off

call cmds\debug_mingw_configure.cmd

cd build_mingw_debug
cmake --build .
cd ..
