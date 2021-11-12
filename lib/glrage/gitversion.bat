@echo off
set version_file=gitversion.h
set version_var=GIT_VERSION
for /f "tokens=*" %%i in ('git describe --tags') do set version_str=%%i
echo #define %version_var% "%version_str%" > %version_file%
