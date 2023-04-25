@echo off
set drctry=%1
if [%1] == [] set drctry=dll
subst x: /d
subst x: c:\
x:
cd x:\svn\samArchive\%drctry%-15(03-04-2023)
set drctry=
