@echo off

setlocal
set OUTDIR=%1
set JSENG=%2
set CYGWIN_ROOT=%~dp0..\..\..\third_party\cygwin\
set GNU_ROOT=%~dp0..\..\..\third_party\gnu\files

set PATH=%CYGWIN_ROOT%bin;%GNU_ROOT%;%SystemRoot%;%SystemRoot%\system32

:: Ensure that the cygwin mount points are defined
CALL %CYGWIN_ROOT%setup_mount.bat > NUL

bash -x create-config.sh %OUTDIR% %JSENG%
