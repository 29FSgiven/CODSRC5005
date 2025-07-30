@echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\Program Files (x86)\mingw-w64\i686-5.3.0-win32-dwarf-rt_v4-rev0\mingw32\bin;%path%
@set binfilename=cod4x_017

set bindir=%LOCALAPPDATA%\CallofDuty4MW
set bindir=%bindir%\bin
set bindir=%bindir%\%binfilename%

exeobfus.exe %bindir%\%binfilename%.dll

pause
exit
