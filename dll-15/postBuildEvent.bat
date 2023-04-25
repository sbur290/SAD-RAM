@echo off
if /i [%1] NEQ [debug] goto :release
    echo copying debug\samDll.dll to emu-%samVersion%\x64\debug and samBug-%samVersion%\bin\x64\debug directories
    copy .\x64\debug\samDll.dll ..\emu-%samVersion%\x64\debug
    copy .\x64\debug\samDll.dll ..\samBug-%samVersion%\bin\x64\debug
    dir ..\samBug-%samVersion%\bin\x64\debug\*.dll |find ".dll"
    goto :xit
:release
    echo copying debug\samDll.dll to \svn\bin and emu-%samVersion%\x64\release directories
    copy .\x64\release\samDll.dll \svn\bin
    copy .\x64\release\samDll.dll ..\emu-%samVersion%\x64\release
    copy .\x64\release\samDll.dll ..\samBug-%samVersion%\bin\x64\release
    dir ..\samBug-%samVersion%\bin\x64\release\*.dll  |find ".dll"
:xit