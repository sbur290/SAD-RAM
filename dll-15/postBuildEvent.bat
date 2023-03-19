@echo off
if /i [%1] NEQ [debug] goto :release
  copy .\x64\debug\samDll.dll ..\emu-15\x64\debug
  goto :xit
:release
  copy .\x64\release\samDll.dll \svn\bin
  copy .\x64\release\samDll.dll ..\emu-15\x64\release

:xit