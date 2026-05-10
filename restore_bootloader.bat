@echo off
REM ===========================================================================
REM Restore the Adafruit nRF52 UF2 bootloader on a wiped chip via J-Link.
REM
REM Usage:
REM   restore_bootloader.bat <path-to-bootloader.hex>
REM
REM Example:
REM   restore_bootloader.bat C:\Users\sqdrc\nice_nano_bootloader-0.9.2_s140_6.1.1.hex
REM
REM Get the hex from
REM   https://github.com/adafruit/Adafruit_nRF52_Bootloader/releases
REM (`nice_nano_bootloader-*.hex` works on most nrf52840 keyboards.)
REM
REM After this succeeds the chip enumerates as a UF2 mass-storage drive on
REM double-tap-RESET, and you can flash ZMK by dropping out\<board>.uf2 onto
REM that drive (i.e. use `build.bat`, not `flash.bat`).
REM ===========================================================================

setlocal

set "HEX=%~1"
if "%HEX%"=="" (
    echo Usage: restore_bootloader.bat ^<path-to-bootloader.hex^>
    exit /b 1
)
if not exist "%HEX%" (
    echo [restore] file not found: %HEX% 1>&2
    exit /b 1
)

set "JLINK=JLink.exe"
where JLink.exe >nul 2>&1
if errorlevel 1 (
    set "JLINK=JLinkExe"
    where JLinkExe >nul 2>&1
    if errorlevel 1 (
        echo [restore] JLink.exe not found on PATH. 1>&2
        exit /b 1
    )
)

set "JCMD=%TEMP%\restore_bootloader.jlink"
> "%JCMD%" echo connect
>>"%JCMD%" echo halt
>>"%JCMD%" echo erase
>>"%JCMD%" echo loadfile %HEX%
>>"%JCMD%" echo r
>>"%JCMD%" echo g
>>"%JCMD%" echo q

echo [restore] flashing bootloader: %HEX%
"%JLINK%" -Device NRF52840_XXAA -If SWD -Speed 1000 -AutoConnect 1 -ExitOnError 1 -NoGui 1 -CommanderScript "%JCMD%"
if errorlevel 1 (
    echo [restore] FAILED 1>&2
    exit /b 1
)

del "%JCMD%" >nul 2>&1
echo.
echo [restore] done. Double-tap RESET on the board to enter UF2 mode,
echo [restore] then drop out\^<variant^>.uf2 onto the appearing drive.
exit /b 0
