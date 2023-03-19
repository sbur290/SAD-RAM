@echo off
cdsim > nul
cd tests
ed -b -t -r "\xB7" "\x00" *.goldMsg
cd ..
