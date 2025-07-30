@echo off
SET FLAGS=-s -Wall -O0 -DNDEBUG
SET FLAGS_O3=-s -O3 -Wall -DNDEBUG

call compiledll_common.cmd

exeobfus.exe %bindir%\%binfilename%.dll

pause
exit
