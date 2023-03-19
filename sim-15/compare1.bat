@echo off
pushd
call cdBackup
c:
compare c:%1 x:%1
popd