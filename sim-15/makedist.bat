@echo off
rem -------- Make distribution directory and zip
set  simdir=c:\svn\sam\sim-15
set destDir=c:\svn\temp\sim-15-dist
if exist %destDir% rd %destDir% /s /q
md %destDir%
cd %destDir%

if exist %simDir%\sim-15.7z   del %simDir%\sim-15.7z
if exist %simDir%\sim-15.hex  del %simDir%\sim-15.hex
copy     %simDir%\..\sim-15\*.*                 .
copy     %simDir%\..\source\c3_*.cpp            .
copy     %simDir%\..\source\bug_sim.cpp         .
copy     %simDir%\..\sam-15\bug_sam.cpp         .
copy     %simDir%\..\source\computeGeometry.cpp .
copy     %simDir%\..\include\c3_*.h             .
copy     %simDir%\..\include\hStructures.h      .
copy     %simDir%\..\include\samParams.h        .
copy     %simDir%\..\sam-15\sambug.*            .
copy     %simdir%\..\sam-15\sam-15.h            .
copy     %simdir%\..\sam-15\samOptions.h        .
copy     %simdir%\..\sam-15\samApp.h            .
copy     %simdir%\..\sam-15\samHelper.h         .
copy     %simdir%\tests\*.sam                   .
copy     %simdir%\*.sam                         .
copy     %verilogSourceDir%\*.sv                .
copy     c:\svn\sam\archive\ver-15\sadram(04-02-2023)\sadram.srcs\sim_1\new\params.txt  .
copy     c:\svn\sam\archive\ver-15\sadram(04-02-2023)\sadram.srcs\sim_1\new\params.data .

if exist *.bat    del *.bat
if exist *.tmp    del *.tmp
if exist *.log    del *.log
if exist todo.txt del todo.txt
copy %simdir%\preBuildEvent.bat          .
copy %verilogSimulationDir%\simulate.log .

for %%f in (*.vcxproj *.filters *.cpp *.h) do call :replace %%f
goto :showDir
:replace
ed -q -t -r "..\x5Csource\x5C"  ".\x5C"                    %1
ed -q -t -r "..\x5Cinclude\x5C" ".\x5C"                    %1
ed -q -t -r "..\x5Csam-15\x5CsamBug.cpp" "samBug.cpp"      %1
ed -q -t -r "..\x5Csam-15\x5CsamBug.h"   "samBug.h"        %1
ed -q -t -r "..\x5Csam-15\x5CsamOptions.h" "samOptions.h"  %1
ed -q -t -r "..\x5Csam-15\x5Csam-15.h" "sam-15.h"          %1
goto :ret
:showDir
dir /w

sim.sln & pause

if exist .vs    rd ".vs" /s /q
if exist x64    rd "x64" /s /q
echo on
cd ..
pause
call 7zip a  sim-15  %destDir%\*.*
bin2hex sim-15.7z
call 7zip l sim-15.7z > sim-15.list
notepad sim-15.list
popd
set simDir=
set destDir=
:ret