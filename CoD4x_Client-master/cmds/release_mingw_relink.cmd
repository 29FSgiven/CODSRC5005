@echo off

del /S /Q build_mingw_release\*.dll build_mingw_release\*.a
call cmds\release_mingw_build.cmd
