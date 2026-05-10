/*
 * Halt-on-fatal override.
 *
 * Default Zephyr behaviour on a fatal error is to log + reset, which
 * bounces straight back into the bootloader and gives no chance to
 * read the panic line over RTT or to inspect the faulting state.
 * Override the handler so a panic marker goes straight to RTT
 * (bypassing the LOG subsystem, which may not be initialised yet
 * during early SYS_INIT) and the CPU stays at a known PC.
 *
 * Compile-time gated by CONFIG_GAMMA_HALT_ON_FATAL.
 */

#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <SEGGER_RTT.h>

#if IS_ENABLED(CONFIG_GAMMA_HALT_ON_FATAL)

static void rtt_put_hex32(uint32_t v) {
    static const char digits[] = "0123456789ABCDEF";
    char buf[10];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[2 + i] = digits[(v >> ((7 - i) * 4)) & 0xF];
    }
    SEGGER_RTT_Write(0, buf, sizeof(buf));
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf) {
    /* Raw RTT writes work even before LOG_INIT — the SEGGER_RTT control
     * block is in .data and exists from the moment Reset_Handler copies
     * .data into RAM, well before any kernel/log subsystem comes up. */
    SEGGER_RTT_WriteString(0, "\n=== GAMMA FATAL ===\nreason=");
    rtt_put_hex32((uint32_t)reason);
    SEGGER_RTT_WriteString(0, "\nesf=");
    rtt_put_hex32((uint32_t)esf);
    SEGGER_RTT_WriteString(0, "\n");

    /* Park forever so RTT keeps the panic frame and JLink halt sees a
     * deterministic PC inside this loop. The stacked exception frame at
     * MSP+24 has the actual fault PC. */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

#endif
