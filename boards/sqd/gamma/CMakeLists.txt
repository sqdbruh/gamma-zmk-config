zephyr_library()
zephyr_library_sources(gamma.c)
zephyr_library_sources(gamma_dongle.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/../zmk/app/include)
