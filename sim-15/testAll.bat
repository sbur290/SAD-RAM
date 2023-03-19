@echo off
set tests=basics gotos loops scin
cls
set options=
set oneFile=
if /i [%1] ==  [/h] goto :help
if /i [%1] NEQ [/?] goto :run

:help
echo --------------- Run various test scenarios -------------------------
echo .bat file to run various .sam files, namely:
echo     %tests%
type \svn\bin\crlf
echo Output is compared to gold standard files saved in .\tests, namely:
echo     *.goldCode,  *.goldMsg, *.goldCap against
echo     *.microCode, *.msgFile, *.capFile respectively.
type \svn\bin\crlf
echo Individual .sam programs will also generate #expect: and #actual messages
echo which are verified by sim.exe and trigger an error if they are not equal.
type \svn\bin\crlf
echo Usage is: testall [options] [list of files]
echo Options:
echo    /xilinx       simulate using the Xilinx simulator xsim.exe (default false)
echo    /emulate      simulate using the Sadram simulator emu.exe  (default true)
echo    /compile      compile .sam programs only
echo    fileName      in combination with the above option, restricts the test to the
echo                  specified file. The aforesaid file is presumed to reside in .\tests
type \svn\bin\crlf
echo Output files are generated in directory %%VerilogSimulationDir%%, ie.
set verilogSimulationDir
goto :xit

rem -----------------------------------------------------------
rem options: fileName set specific file to test; eg.,
:run
   if /i [%1] == [/xilinx]  set options=/xilinx&  shift
   if /i [%1] == [/compile] set options=/compile& shift & goto :run
   if    [%1] == []         goto cannedList
   rem set list of files from command line
   set tests=%1 %2 %3 %4 %5 %6 %7 %8
   if [%2] == [] set oneFile=true
:cannedList
   for %%f in (%tests%) do call :oneTest %%f
   goto :xit

rem -----------------------------------------------------------------------
:oneTest
   call cdsim > nul
   if not exist %samSourceDir%\%1.sam echo %samSourceDir%\%1.sam does not exist&goto :xit
   set microCode=%verilogSimulationDir%\%1.microcode
   set   capture=%verilogSimulationDir%\%1.capFile
   set   msgFile=%verilogSimulationDir%\%1.msgFile
   echo sim %samSourceDir%\%1.sam  /include "%samSourceDir%" %options%
   sim      "%samSourceDir%\%1.sam"  /include "%samSourceDir%" %options% > nul
   if errorlevel 1 goto err
   
   cd tests
   if exist %capture%   compare %1.goldCap  %capture%   /~space /~case /j "##"
   if exist %microcode% compare %1.goldCode %microcode% /~space /~case /j "//File"
   if exist %msgFile%   compare %1.goldMsg  %msgFile%
   goto :ret

:err
echo ******** sim.exe returned errorlevel=%errorlevel% running %1.sam
if [%oneFile%] == [true] goto :xit
set /p yesno=Press Y to continue N to stop testall:
if /i [%yesno%] == [y] goto :ret
call :restore
if [intentional-crash-of-this-bat-file

:restore
call cdsim > nul
set tests=
set microCode=
set capture=
set msgFile=
set yesno=
set options=
set oneFile=
goto :ret

:xit
call :restore
:ret
