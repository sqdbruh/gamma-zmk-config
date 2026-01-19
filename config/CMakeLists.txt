target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/gamma.c
    ${CMAKE_CURRENT_LIST_DIR}/gamma_dongle.c
    ${CMAKE_CURRENT_LIST_DIR}/gamma_led_test.c
    ${CMAKE_CURRENT_LIST_DIR}/gamma_led_probe.c
)

if (CONFIG_GAMMA_LED_PROBE)
    if (NOT Python3_EXECUTABLE)
        set(Python3_EXECUTABLE python3)
    endif()

    set(GAMMA_PROBE_MARKER "GAMMA_PROBE_V1_2026_01_19")

    add_custom_command(
        TARGET zephyr_final
        POST_BUILD
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/verify_marker.py
                ${CMAKE_BINARY_DIR} ${GAMMA_PROBE_MARKER}
        VERBATIM
    )
endif()
