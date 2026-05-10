zephyr_library()
zephyr_library_sources(src/behaviors/behavior_check_battery.c)
zephyr_library_sources_ifdef(CONFIG_GAMMA_HALT_ON_FATAL src/fatal.c)
zephyr_include_directories(include)
# ZMK's behavior driver API lives under app/include in the upstream tree.
# West clones ZMK as a project alongside Zephyr, so ZEPHYR_BASE/../zmk/app
# resolves to ZMK's application directory.
zephyr_library_include_directories(${ZEPHYR_BASE}/../zmk/app/include)
