set(GAMMA_CONFIG_DIR ${CMAKE_CURRENT_LIST_DIR}/../config)

target_sources(app PRIVATE
    ${GAMMA_CONFIG_DIR}/gamma.c
    ${GAMMA_CONFIG_DIR}/gamma_dongle.c
    ${GAMMA_CONFIG_DIR}/gamma_led_test.c
    ${GAMMA_CONFIG_DIR}/gamma_led_probe.c
)

if (CONFIG_GAMMA_LED_PROBE)
    set(GAMMA_PROBE_MARKER "GAMMA_PROBE_V1_2026_01_19")
    if (NOT Python3_EXECUTABLE)
        find_package(Python3 COMPONENTS Interpreter)
    endif()
    if (NOT Python3_EXECUTABLE)
        set(Python3_EXECUTABLE python3)
    endif()

    set(GAMMA_MARKER_STAMP ${CMAKE_BINARY_DIR}/gamma_marker_check.stamp)
    add_custom_command(
        OUTPUT ${GAMMA_MARKER_STAMP}
        COMMAND ${Python3_EXECUTABLE} ${GAMMA_CONFIG_DIR}/verify_marker.py
                ${CMAKE_BINARY_DIR} ${GAMMA_PROBE_MARKER}
        COMMAND ${CMAKE_COMMAND} -E touch ${GAMMA_MARKER_STAMP}
        DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zmk.uf2
        VERBATIM
    )
    add_custom_target(gamma_marker_check ALL DEPENDS ${GAMMA_MARKER_STAMP})
endif()
