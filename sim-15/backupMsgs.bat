@echo off
call cdbackup > nul
copy c:tests\*.goldMsg x:tests
c:
