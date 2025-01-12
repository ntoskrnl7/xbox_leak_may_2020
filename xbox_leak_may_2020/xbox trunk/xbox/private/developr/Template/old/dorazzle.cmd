@REM set _echo=1
@if NOT DEFINED _echo echo off

REM *****************************
REM *** DO NOT EDIT THIS FILE ***
REM *****************************

REM *************************************************************************
REM *
REM * DoRazzle.cmd
REM *
REM * Creates a Razzle window.
REM *
REM * Usage:
REM *   DoRazzle [drive_letter [NTROOT directory]] [WIN64] {SetChk | SetFre} [params]
REM *
REM * Examples:
REM *   DoRazzle D: SetChk
REM *   DoRazzle D: ntroot \nt.sp4 SetFre
REM *   DoRazzle D: win64 SetChk NoOpt
REM *
REM *************************************************************************

REM
REM Set _NTDrive
REM
if "%1" == "" set _NTDrive=C:
if not "%1" == "" set _NTDrive=%1& shift

REM
REM Set _NTRoot
REM
set _NTRoot=\nt
if /i "%1" == "ntroot" if not "%2" == "" (
    set _NTRoot=%2
    shift
    shift
)

REM
REM Support Win64
REM
if /i "%1" == "WIN64" (
	set _NTWIN64=1
	shift
)

REM
REM Add MSTOOLS, IDW and PUBLIC\TOOLS to the path if not in system directory.
REM
if exist %_NTDrive%%_NTRoot%\mstools (
    set MSTOOLS_DIR=%_NTDrive%%_NTRoot%\mstools
    set PATH=%PATH%;%_NTDrive%%_NTRoot%\mstools
)
if exist %_NTDrive%%_NTRoot%\idw (
    set IDW_DIR=%_NTDrive%%_NTRoot%\idw
    set PATH=%PATH%;%_NTDrive%%_NTRoot%\idw
)
if exist %_NTDrive%%_NTRoot%\public\tools (
    set PATH=%PATH%;%_NTDrive%%_NTRoot%\public\tools
)

REM
REM Make sure check or free is specified.
REM
if NOT "%1" == "" goto INVOKEIT
echo You must specify either SetChk or SetFre to DoRazzle.cmd.
pause
goto DONE

:INVOKEIT
REM
REM Invoke RAZZLE.CMD, which does the real work of setting up the Razzle window.
REM
%_NTDrive%%_NTRoot%\public\tools\razzle.cmd %1 %2 %3 %4 %4 %6 %7 %8

:DONE
