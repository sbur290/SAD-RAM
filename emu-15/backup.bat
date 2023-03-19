@echo off
REM ---------- create environment variable backupDate = day-month-year %02-%02-%04d
call backupDate

set backupName=..\archive\emu-15(%backupDate%)
set backupDate=

REM ---------- check directory does not already exist
if not exist %backupName% goto :ok
:what
set /p ok=Directory %backupName% already exist. Continue ? (Y/N)
if [%ok%] == [y] goto ok
if [%ok%] == [n] goto :final
                 goto :what

:ok
REM ---------- create directory and copy files
echo Backing up to %backupName%
echo "List of files copied by backup.bat" > copySave.tmp
xcopy .\*.* %backupName% /s /i /y        >> copySave.tmp
REM ---------- delete unwanted files and directories in %backupName%
rd %backupName%\x64 /q /s
del %backupName%\*.exe
del %backupName%\*.tmp
if exist *.dump del *.dump

REM ---------- Display backed up directory
dir %backupName%
:xit
REM ---------- change compareWithBackup.bat to backup directory
echo @subst x: /d>         cdBackup.bat
echo @subst x: c:\>>       cdBackup.bat
echo @x:>>                 cdBackup.bat
echo @cd x:%backupName%>>  cdBackup.bat
ed -t -q -r "x:..\x5C" "x:\x5Csvn\x5Csam\x5C" cdBackup.bat

copy compareWithBackup.bat %backupname% /y
copy compare1.bat          %backupname% /y
copy cdBackup.bat          %backupname% /y

:final
set backupName=
set ok=