@echo off
REM ===========================================================================
REM Local SWD flash helper (J-Link).
REM
REM Usage:
REM   flash.bat left
REM   flash.bat right
REM   flash.bat dongle
REM   flash.bat -p left           force pristine rebuild before flashing
REM
REM Builds the variant with swd.conf applied (vectors at 0x0, UF2 disabled)
REM into build\gamma_<variant>_swd\, then erases + programs + resets the chip
REM via JLinkExe over SWD.
REM
REM Use this after `nrfjprog --recover` wiped the UF2 bootloader, or any time
REM you don't want to go through the bootloader to flash. Halves and the
REM dongle that still have the Adafruit UF2 bootloader can keep using
REM build.bat + dropping the .uf2 onto the bootloader drive.
REM
REM Requires JLinkExe on PATH (SEGGER J-Link tools).
REM ===========================================================================

setlocal EnableDelayedExpansion

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "PRISTINE="
set "TARGETS="

:parse
if "%~1"=="" goto after_parse
if /I "%~1"=="-p"        ( set "PRISTINE=-p" & shift & goto parse )
if /I "%~1"=="--pristine" ( set "PRISTINE=-p" & shift & goto parse )
if /I "%~1"=="left"      ( set "TARGETS=!TARGETS! gamma_left" & shift & goto parse )
if /I "%~1"=="right"     ( set "TARGETS=!TARGETS! gamma_right" & shift & goto parse )
if /I "%~1"=="dongle"    ( set "TARGETS=!TARGETS! gamma_dongle" & shift & goto parse )
echo [flash] unknown argument: %~1 1>&2
exit /b 2

:after_parse
if "%TARGETS%"=="" (
    echo [flash] usage: flash.bat ^<left^|right^|dongle^> [-p]
    exit /b 1
)

set "JLINK=JLink.exe"
where JLink.exe >nul 2>&1
if errorlevel 1 (
    set "JLINK=JLinkExe"
    where JLinkExe >nul 2>&1
    if errorlevel 1 (
        echo [flash] JLink.exe not found on PATH. 1>&2
        echo [flash] install SEGGER J-Link tools and retry. 1>&2
        exit /b 1
    )
)

if not exist "%ROOT%\.west\config" (
    echo [flash] west workspace not initialised. Run build.bat once first. 1>&2
    exit /b 1
)

for %%T in (%TARGETS%) do (
    call :flash_one %%T
    if errorlevel 1 exit /b 1
)
exit /b 0


:flash_one
set "BOARD=%~1"
set "BUILD_DIR=%ROOT%\build\%BOARD%_swd"
set "HEX=%BUILD_DIR%\zephyr\zmk.hex"
if not exist "%HEX%" set "HEX=%BUILD_DIR%\zephyr\zephyr.hex"

echo.
echo ============================================================
echo [flash] %BOARD%  ^(SWD, no bootloader^)
echo ============================================================

pushd "%ROOT%" || exit /b 1
call west build %PRISTINE% -d "%BUILD_DIR%" -s zmk/app -b "%BOARD%" -- -DZMK_CONFIG="%ROOT%\config" -DZMK_EXTRA_MODULES="%ROOT%" -DEXTRA_CONF_FILE="%ROOT%\swd.conf"
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" (
    echo [flash] %BOARD% build FAILED ^(exit %RC%^) 1>&2
    exit /b %RC%
)

if not exist "%HEX%" (
    echo [flash] expected hex not found: %HEX% 1>&2
    exit /b 1
)

set "JCMD=%TEMP%\flash_%BOARD%.jlink"
> "%JCMD%" echo r
>>"%JCMD%" echo h
>>"%JCMD%" echo erase
>>"%JCMD%" echo loadfile %HEX%
>>"%JCMD%" echo r
>>"%JCMD%" echo g
>>"%JCMD%" echo q

echo [flash] flashing via JLinkExe (script: %JCMD%)
"%JLINK%" -Device NRF52840_XXAA -If SWD -Speed 4000 -AutoConnect 1 -ExitOnError 1 -NoGui 1 -CommanderScript "%JCMD%"
if errorlevel 1 (
    echo [flash] %BOARD% JLink flash FAILED 1>&2
    exit /b 1
)

del "%JCMD%" >nul 2>&1
echo [flash] %BOARD% done.
exit /b 0
