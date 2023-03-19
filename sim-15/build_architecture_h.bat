@echo off
copy %verilogSourceDir%\architecture.sv architecture.patch > nul
ed -q    -r "$clog2" "MyLog2"           architecture.patch
ed -q -t -r "2 **"     "2ull \x3C\x3C"  architecture.patch
ed -q    -r " = "      ","              architecture.patch
ed -q    -r ";"      "); "             architecture.patch
ed -q    -r "parameter " "parameter("   architecture.patch
ed -q -r "bits  ," "bits  ="            architecture.patch
ed -q -p architecture.cpp "#1"          architecture.patch 
del architecture.patch 

call cdv src > nul
ed -q -p samDefines.sv "#1"             architecture.sv
call cdsim > nul 
echo architecture.cpp patched with values from 
echo       %verilogSourceDir%\architecture.sv
