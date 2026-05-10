/*
 * Halt-on-fatal override.
 *
 * Default Zephyr behaviour on a fatal error is to log + reset, which
 * bounces straight back into the bootloader and gives no chance to
 * read the panic line over RTT or to inspect the faulting state with
 * a debugger. Override the handler so the panic dump goes straight
 * to RTT (bypassing the LOG subsystem, which may not be initialised
 * if the fault happens during early SYS_INIT) and the CPU stays at
 * a known PC.
 *
 * Compile-time gated by CONFIG_GAMMA_HALT_ON_FATAL.
 */

#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <SEGGER_RTT.h>

#if IS_ENABLED(CONFIG_GAMMA_HALT_ON_FATAL)

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf) {
    /* Raw RTT writes — usable even before the LOG subsystem comes up,
     * unlike LOG_ERR. Channel 0 is what JLinkRTTViewer reads. */
    SEGGER_RTT_printf(0, "\n=== GAMMA FATAL ===\n");
    SEGGER_RTT_printf(0, "reason: %u\n", reason);
    if (esf != NULL) {
        SEGGER_RTT_printf(0, "esf @ %p\n", (void *)esf);
    }
    /* Park forever so RTT keeps the panic frame and JLink halt sees a
     * deterministic PC inside this loop. The stacked exception frame at
     * MSP+24 has the actual fault PC. */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

#endif
