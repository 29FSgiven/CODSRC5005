@echo off

del /S /Q build_mingw_debug\*.dll build_mingw_debug\*.a
call cmds\debug_mingw_build.cmd
