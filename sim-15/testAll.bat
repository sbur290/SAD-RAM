@echo off
set tests=basics gotos loops scin
cls
set options=
set oneFile=
if /i [%1] ==  [/h] goto :help
if /i [%1] NEQ [/?] goto :run

:help
echo --------------- .bat file to Run various test scenarios -----------------
echo runs sim against file %tests%
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
goto :xit

rem -----------------------------------------------------------
rem splitup options and file names
:run
   set oneFile=
   if    [%1] == []         goto cannedList
   set oneFile=true
   set options=
   set files=
   set multiFile=
   for %%f in (%*)      do call :options %%f
   if [%multiFile%] == [] set files=%tests%
   for %%f in (%files%) do call :oneTest %%f
   goto :xit
:cannedList --------
   for %%f in (%tests%) do call :oneTest %%f
   goto :xit

:options -------- extract options
   if /i [%1] == [/xilinx]  set options=%options% %1&  goto :ret
   if /i [%1] == [/compile] set options=%options% %1&  goto :ret
   if [%multifile%] NEQ [] set oneFile=
   set multiFile=yes
   if not exist %samSourceDir%\%1.sam echo %samSourceDir%\%1.sam does not exist&goto :crash
   set files=%files% %1&                               goto :ret

rem -----------------------------------------------------------------------
:oneTest
   call cdsim > nul
   set microCode=%verilogSimulationDir%\%1.microcode
   set   capture=%verilogSimulationDir%\%1.capFile
   set   msgFile=%verilogSimulationDir%\%1.msgFile
   echo sim  %samSourceDir%\%1.sam  /include "%samSourceDir%" %options%
        sim "%samSourceDir%\%1.sam" /include "%samSourceDir%" %options% > nul
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
:crash
if [intentional-crash-of-this-bat-file

:restore
call cdsim > nul
set files=
set tests=
set microCode=
set capture=
set msgFile=
set yesno=
set options=
set oneFile=
set multiFile=
goto :ret

:xit
call :restore
:ret
