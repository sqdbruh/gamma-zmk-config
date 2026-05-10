# gamma-zmk-config

ZMK firmware config for the **Gamma** keyboard — a 3-part split (left + right +
USB dongle / central) built around the Nordic nRF52840 (QIAA).

This repo is both a **zmk-config** (the keymap/conf you'd edit) and a **ZMK
keyboard module** (it ships the board files for `gamma_left`, `gamma_right`,
and `gamma_dongle`, plus a custom `&check_bat` behavior). Building it does
not require any patches to the upstream ZMK tree.

## Repository layout

```
.
├── boards/sqd/gamma/                Board files for all three Gamma variants
│   ├── Kconfig.board                CONFIG_BOARD_GAMMA_{LEFT,RIGHT,DONGLE}
│   ├── Kconfig.defconfig            Defaults conditional on the active variant
│   ├── gamma.dtsi                   Shared SoC + matrix + LED-strip nodes
│   ├── gamma_halves.dts             Shared between left and right halves
│   ├── gamma_left.dts / gamma_right.dts / gamma_dongle.dts
│   ├── gamma_*_defconfig            Per-variant Kconfig defaults
│   ├── gamma.c                      LED-strip animations + battery UI (halves)
│   ├── gamma_dongle.c               Dongle-side LED handling (currently #if 0)
│   ├── gamma_seg.c                  Optional 7-segment driver (not built)
│   └── gamma.keymap                 Default keymap shipped with the board
├── config/                          User-editable zmk-config
│   ├── west.yml                     Pulls in upstream zmkfirmware/zmk
│   ├── gamma.keymap                 Overrides the board default
│   └── gamma.conf                   Project-wide Kconfig overrides
├── dts/
│   ├── behaviors/check_battery.dtsi check_bat node included by the keymap
│   └── bindings/behaviors/zmk,behavior-check-battery.yaml
├── include/zmk/check_battery.h      show_battery() / hide_battery() prototypes
├── src/behaviors/behavior_check_battery.c   Custom &check_bat behavior driver
├── zephyr/module.yml                Declares this repo as a Zephyr module
├── Kconfig                          Module-level Kconfig (ZMK_CHECK_BATTERY_BEH)
├── CMakeLists.txt                   Module-level CMake (compiles the behavior)
├── build.yaml                       GitHub Actions build matrix
└── .github/workflows/build.yml      Calls zmkfirmware's reusable workflow
```

## Building the firmware

### Cloud build (recommended for end users)

1. Fork this repo on GitHub.
2. Push a commit. The workflow runs and produces a `firmware.zip` artifact
   containing three `.uf2` files: `gamma_left-zmk.uf2`,
   `gamma_right-zmk.uf2`, `gamma_dongle-zmk.uf2`.
3. Flash each half / the dongle by dropping its UF2 onto the corresponding
   bootloader drive.

### Local build (Windows)

`build.bat` at the repo root does first-time `west init`/`update` and
then builds whatever variants you ask for. Requires the ZMK toolchain
(Python + west + cmake + ninja + Zephyr SDK) on `PATH` — see
https://zmk.dev/docs/development/setup/toolchains.

```bat
build.bat                  REM build all three: left, right, dongle
build.bat dongle           REM single variant
build.bat left right       REM halves only
build.bat -p dongle        REM force pristine rebuild
build.bat clean            REM wipe build/, out/, .west/, zephyr/, zmk/, modules/
```

UF2s land in `out\<board>.uf2` (`out\gamma_left.uf2` etc.). Drop each
onto the matching board's bootloader drive to flash.

### Restoring the UF2 bootloader after `nrfjprog --recover`

`nrfjprog --recover` wipes the Adafruit UF2 bootloader at flash 0x0.
To get the drag-and-drop flow back, flash the bootloader hex via SWD:

1. Download a bootloader hex from
   [Adafruit_nRF52_Bootloader releases](https://github.com/adafruit/Adafruit_nRF52_Bootloader/releases).
   `nice_nano_bootloader-*.hex` works on most nrf52840 keyboards.
2. `restore_bootloader.bat C:\path\to\nice_nano_bootloader-*.hex`
3. Double-tap the board's RESET button — it should enumerate as a USB
   mass-storage drive.
4. Build normally: `build.bat <variant>` (no `swd.conf`, code at the
   bootloader's expected partition).
5. Drop `out\<variant>.uf2` onto the drive.

### SWD flashing (no bootloader)

If a chip's UF2 bootloader is gone (e.g. after `nrfjprog --recover`),
or you just don't want to go through the bootloader, use `flash.bat`:

```bat
flash.bat left           REM build + flash via SWD/nrfjprog
flash.bat dongle
flash.bat -p right       REM force pristine rebuild before flashing
```

This builds with `swd.conf` applied (vectors at 0x0, UF2 disabled)
into `build\gamma_<variant>_swd\` and runs
`nrfjprog --eraseall && nrfjprog --program <zephyr.hex> && nrfjprog --reset`.
Requires nRF Command Line Tools on `PATH`.

### Local build (manual)

```sh
west init -l config
west update
west zephyr-export

west build -d build/left   -s zmk/app -b gamma_left   -- -DZMK_CONFIG="$(pwd)/config" -DZMK_EXTRA_MODULES="$(pwd)"
west build -d build/right  -s zmk/app -b gamma_right  -- -DZMK_CONFIG="$(pwd)/config" -DZMK_EXTRA_MODULES="$(pwd)"
west build -d build/dongle -s zmk/app -b gamma_dongle -- -DZMK_CONFIG="$(pwd)/config" -DZMK_EXTRA_MODULES="$(pwd)"
```

UF2 files land in `build/<variant>/zephyr/zmk.uf2`.

## Customizing

- **Keymap:** edit `config/gamma.keymap`. The shipped default lives at
  `boards/sqd/gamma/gamma.keymap` and is overridden whenever
  `config/gamma.keymap` exists.
- **Project-wide Kconfig:** edit `config/gamma.conf`.
- **Per-variant Kconfig:** edit the matching `boards/sqd/gamma/gamma_<v>_defconfig`.

## Notes on the `&check_bat` behavior

`&check_bat` is a custom zero-param behavior shipped by this module. When
pressed it calls `show_battery()`, on release it calls `hide_battery()`.
Implementations live in `boards/sqd/gamma/gamma.c` (LED-strip variant) and
`boards/sqd/gamma/gamma_seg.c` (7-segment variant — disabled by default).
The behavior compiles to no-ops when `CONFIG_ZMK_CHECK_BATTERY_BEH=n`, so
other boards in this module can leave the symbol off.
