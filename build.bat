@echo off
REM ===========================================================================
REM Local build helper for gamma-zmk-config.
REM
REM Usage:
REM   build.bat                 build left + right + dongle (normal firmware)
REM   build.bat dongle          build just the dongle
REM   build.bat left right      build halves only
REM   build.bat -r              build settings-reset firmware for all three
REM   build.bat -r dongle       settings-reset firmware for the dongle only
REM   build.bat -p ...          force pristine for the listed targets
REM   build.bat clean           wipe build/, out/, .west/, west clones
REM
REM First run does `west init -l config && west update && west zephyr-export`,
REM cloning zmk + zephyr + modules into the repo root. Subsequent runs skip
REM init and just rebuild. UF2s land in .\out\.
REM
REM Settings-reset firmware (-r) flashes once to wipe BLE bonds and the ZMK
REM settings store; reflash the regular firmware afterwards. Use it when you
REM need to re-pair halves with the dongle from a clean state.
REM
REM Requires the ZMK toolchain on PATH: python with `west`, `cmake`, `ninja`,
REM and the Zephyr ARM SDK (ZEPHYR_SDK_INSTALL_DIR set or sdk in default path).
REM See https://zmk.dev/docs/development/setup/toolchains.
REM ===========================================================================

setlocal EnableDelayedExpansion

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "PRISTINE="
set "RESET_MODE="
set "TARGETS="

:parse
if "%~1"=="" goto after_parse
if /I "%~1"=="-p"        ( set "PRISTINE=-p" & shift & goto parse )
if /I "%~1"=="--pristine" ( set "PRISTINE=-p" & shift & goto parse )
if /I "%~1"=="-r"        ( set "RESET_MODE=1" & shift & goto parse )
if /I "%~1"=="--reset"   ( set "RESET_MODE=1" & shift & goto parse )
if /I "%~1"=="clean"     goto do_clean
if /I "%~1"=="left"      ( set "TARGETS=!TARGETS! gamma_left" & shift & goto parse )
if /I "%~1"=="right"     ( set "TARGETS=!TARGETS! gamma_right" & shift & goto parse )
if /I "%~1"=="dongle"    ( set "TARGETS=!TARGETS! gamma_dongle" & shift & goto parse )
if /I "%~1"=="all"       ( set "TARGETS=gamma_left gamma_right gamma_dongle" & shift & goto parse )
echo [build] unknown argument: %~1 1>&2
exit /b 2

:after_parse
if "%TARGETS%"=="" set "TARGETS=gamma_left gamma_right gamma_dongle"

if not exist "%ROOT%\.west\config" (
    echo [build] west workspace not initialised, running first-time setup...
    pushd "%ROOT%" || exit /b 1
    call west init -l config
    if errorlevel 1 ( popd & echo [build] west init failed & exit /b 1 )
    call west update
    if errorlevel 1 ( popd & echo [build] west update failed & exit /b 1 )
    call west zephyr-export
    if errorlevel 1 ( popd & echo [build] west zephyr-export failed & exit /b 1 )
    popd
)

if not exist "%ROOT%\out" mkdir "%ROOT%\out"

set "BUILD_FAILED="
for %%T in (%TARGETS%) do (
    call :build_one %%T
    if errorlevel 1 set "BUILD_FAILED=1"
)

if defined BUILD_FAILED (
    echo.
    echo [build] one or more targets failed. 1>&2
    exit /b 1
)

echo.
echo [build] done. Artifacts in %ROOT%\out\:
dir /b "%ROOT%\out\*.uf2"
exit /b 0


:build_one
set "BOARD=%~1"
if defined RESET_MODE (
    set "SUFFIX=_reset"
    set "EXTRA_CMAKE=-DEXTRA_CONF_FILE=%ROOT%\settings_reset.conf"
) else (
    set "SUFFIX="
    set "EXTRA_CMAKE="
)
set "BUILD_DIR=%ROOT%\build\%BOARD%%SUFFIX%"
set "OUT_NAME=%BOARD%%SUFFIX%"

echo.
echo ============================================================
echo [build] %OUT_NAME%
echo ============================================================

REM Dongle is the central — apply the studio-rpc-usb-uart snippet so
REM ZMK Studio gets a USB CDC RPC endpoint. Halves don't need it.
set "SNIPPET="
if /I "%BOARD%"=="gamma_dongle" set "SNIPPET=-S studio-rpc-usb-uart"

pushd "%ROOT%" || exit /b 1
call west build %PRISTINE% -d "%BUILD_DIR%" -s zmk/app -b "%BOARD%" %SNIPPET% -- -DZMK_CONFIG="%ROOT%\config" -DZMK_EXTRA_MODULES="%ROOT%" %EXTRA_CMAKE%
set "RC=%ERRORLEVEL%"
popd

if not "%RC%"=="0" (
    echo [build] %OUT_NAME% FAILED ^(exit %RC%^) 1>&2
    exit /b %RC%
)

if not exist "%BUILD_DIR%\zephyr\zmk.uf2" (
    echo [build] %OUT_NAME% built but no zmk.uf2 found at %BUILD_DIR%\zephyr\ 1>&2
    echo [build] retry with: build.bat -p %BOARD:gamma_=% 1>&2
    exit /b 1
)
REM Sanity-check: real ZMK firmware is ~150-300 KB. Anything tiny is stale.
for %%S in ("%BUILD_DIR%\zephyr\zmk.uf2") do set "UF2_SIZE=%%~zS"
if %UF2_SIZE% LSS 10000 (
    echo [build] %OUT_NAME%: zmk.uf2 is only %UF2_SIZE% bytes — looks stale. 1>&2
    echo [build] retry with: build.bat -p %BOARD:gamma_=% 1>&2
    exit /b 1
)
copy /Y "%BUILD_DIR%\zephyr\zmk.uf2" "%ROOT%\out\%OUT_NAME%.uf2" >nul
echo [build] %OUT_NAME% -> out\%OUT_NAME%.uf2 ^(%UF2_SIZE% bytes^)
exit /b 0


:do_clean
echo [build] cleaning...
if exist "%ROOT%\build" rmdir /s /q "%ROOT%\build"
if exist "%ROOT%\out"   rmdir /s /q "%ROOT%\out"
if exist "%ROOT%\.west" rmdir /s /q "%ROOT%\.west"
if exist "%ROOT%\zephyr" (
    REM Don't blow away zephyr/module.yml — only nuke if a west-clone Zephyr lives here.
    if exist "%ROOT%\zephyr\Kconfig" rmdir /s /q "%ROOT%\zephyr"
)
if exist "%ROOT%\zmk"     rmdir /s /q "%ROOT%\zmk"
if exist "%ROOT%\modules" rmdir /s /q "%ROOT%\modules"
if exist "%ROOT%\bootloader" rmdir /s /q "%ROOT%\bootloader"
if exist "%ROOT%\tools"   rmdir /s /q "%ROOT%\tools"
echo [build] clean done.
exit /b 0
