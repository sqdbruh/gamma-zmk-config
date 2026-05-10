@echo off
REM ===========================================================================
REM Local build helper for gamma-zmk-config.
REM
REM Usage:
REM   build.bat                 build left + right + dongle
REM   build.bat dongle          build just the dongle
REM   build.bat left right      build halves only
REM   build.bat clean           wipe build/ and out/ and re-init west
REM   build.bat -p ...          add -p to force pristine for the listed targets
REM
REM First run does `west init -l config && west update && west zephyr-export`,
REM cloning zmk + zephyr + modules into the repo root. Subsequent runs skip
REM init and just rebuild. UF2s land in .\out\.
REM
REM Requires the ZMK toolchain on PATH: python with `west`, `cmake`, `ninja`,
REM and the Zephyr ARM SDK (ZEPHYR_SDK_INSTALL_DIR set or sdk in default path).
REM See https://zmk.dev/docs/development/setup/toolchains.
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
set "BUILD_DIR=%ROOT%\build\%BOARD%"
echo.
echo ============================================================
echo [build] %BOARD%
echo ============================================================

pushd "%ROOT%" || exit /b 1
call west build %PRISTINE% -d "%BUILD_DIR%" -s zmk/app -b "%BOARD%" -- -DZMK_CONFIG="%ROOT%\config" -DZMK_EXTRA_MODULES="%ROOT%"
set "RC=%ERRORLEVEL%"
popd

if not "%RC%"=="0" (
    echo [build] %BOARD% FAILED ^(exit %RC%^) 1>&2
    exit /b %RC%
)

if exist "%BUILD_DIR%\zephyr\zmk.uf2" (
    copy /Y "%BUILD_DIR%\zephyr\zmk.uf2" "%ROOT%\out\%BOARD%.uf2" >nul
    echo [build] %BOARD% -> out\%BOARD%.uf2
) else (
    echo [build] %BOARD% built but no zmk.uf2 found at %BUILD_DIR%\zephyr\ 1>&2
    exit /b 1
)
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
