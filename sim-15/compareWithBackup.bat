@echo off
Rem compare all critical files with backup version in backup directory
set thisDir=%cd%
call cdbackup& rem sets drive x: to backup directory
set someDifferences=
echo %date% > compareSave.tmp
c:
for %%f in (*.cpp)     do call :compareOne %%f
for %%f in (*.h  )     do call :compareOne %%f
for %%f in (*.sln)     do call :compareOne %%f
for %%f in (*.vcxproj) do call :compareOne %%f
for %%f in (*.bat)     do call :compareOne %%f
for %%f in (*.txt)     do call :compareOne %%f
cd tests
for %%f in (*.*)       do call :compareOne tests\%%f
cd ..
if [%someDifferences%] NEQ [] goto :bye
x:
set xcd=%cd%
echo no differences between %thisDir%
echo                and     %xcd% encountered
c:

:bye
set xcd=
set thisDir=
set someDifferences=
goto :eof

:compareOne
 if [%~x1] == [.tmp] goto :eof
 rem echo on&echo c:\svn\bin\compare.exe %thisDir%\%1 x:%1& echo off&pause
 c:\svn\bin\compare %thisDir%\%1 x:%1
 if [%ERRORLEVEL%] equ [0] echo Compare %1 >> %thisDir%\compareSave.tmp &goto :eof
 c:\svn\bin\wincmp3 %thisDir%\%1 x:%1
 echo Differences %1 >> compareSave.tmp
 set someDifferences=yes
 goto :eof

:eof
