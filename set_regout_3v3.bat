@echo off
REM ===========================================================================
REM Write nRF52840 UICR.REGOUT0 = 3.3V via J-Link.
REM
REM Background: the chip's internal regulator drives VDD. On a fresh or
REM `nrfjprog --recover`-wiped UICR the field defaults to 1.8V. The MCU core
REM happily runs at 1.8V but anything 3V3-spec'd on the board (WS2812 LEDs,
REM charging IC, BLE PA, sensors) is starved — the typical symptom is
REM "everything works while J-Link is attached, freezes the moment SWD is
REM detached" because J-Link's VTref pin was supplying 3V3 externally.
REM
REM This is a one-shot fix per chip. UICR is non-volatile, the value sticks
REM until the next chip-erase / nrfjprog --recover. After the write a full
REM power cycle is required for the regulator to actually start outputting
REM the new voltage.
REM
REM REGOUT0 layout (nRF52840 PS, register at 0x10001304):
REM   bits [2:0] VOUT  — 0=1.8V, 1=2.1V, 2=2.4V, 3=2.7V, 4=3.0V, 5=3.3V
REM   bits [31:3]      — reserved, must remain 1
REM Erased UICR = 0xFFFFFFFF. UICR bits can only be cleared (1->0) without
REM erasing first, so we write 0xFFFFFFFD = ...1111_1101 to set VOUT=0b101=5.
REM
REM Usage: set_regout_3v3.bat
REM Requires JLink.exe on PATH.
REM ===========================================================================

setlocal

set "JLINK=JLink.exe"
where JLink.exe >nul 2>&1
if errorlevel 1 (
    set "JLINK=JLinkExe"
    where JLinkExe >nul 2>&1
    if errorlevel 1 (
        echo [regout] JLink.exe not found on PATH. 1>&2
        exit /b 1
    )
)

set "JCMD=%TEMP%\set_regout_3v3.jlink"
> "%JCMD%" echo connect
>>"%JCMD%" echo halt
REM Read current value first so we can see whether it's already programmed.
>>"%JCMD%" echo mem32 0x10001304 1
REM REGOUT0 = 0xFFFFFFFD -> VOUT=5 (3.3V), all reserved bits stay at 1.
>>"%JCMD%" echo w4 0x10001304 0xFFFFFFFD
REM Read back to confirm.
>>"%JCMD%" echo mem32 0x10001304 1
>>"%JCMD%" echo r
>>"%JCMD%" echo g
>>"%JCMD%" echo q

echo [regout] writing UICR.REGOUT0 = 3.3V
"%JLINK%" -Device NRF52840_XXAA -If SWD -Speed 1000 -AutoConnect 1 -ExitOnError 1 -NoGui 1 -CommanderScript "%JCMD%"
if errorlevel 1 (
    echo [regout] FAILED 1>&2
    exit /b 1
)

del "%JCMD%" >nul 2>&1
echo.
echo [regout] done. UNPLUG ALL POWER (battery + USB + J-Link) for ~5s,
echo [regout] then reapply power. The regulator only switches output
echo [regout] voltage on a full power-on reset, not on a soft reset.
exit /b 0
