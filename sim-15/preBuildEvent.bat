@echo off
buildErrors ..\source-15\c3_errors.cpp ..\include-15\c3_errors.h /q
echo set theTime=%time% > c:\svn\temp.bat
ed -q -r ":"       "-"           c:\svn\temp.bat >nul
ed -q -r "."       "-"           c:\svn\temp.bat >nul
ed -q -r "theTime= " "theTime=0" c:\svn\temp.bat >nul

call c:\svn\temp
del c:\svn\temp.bat

set dir=c:\svn\sam\ver-15\sadram\sadram.sim\sim_1\behav\xsim
if exist %dir%\bug-sim.cap     copy   %dir%\bug-sim.cap  %dir%\bug-sim.%theTime%

set theTime=
set dir=
