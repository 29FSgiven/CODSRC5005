@echo off

call cmds\release_mingw_configure.cmd

cd build_mingw_release
cmake --build .
cd ..
