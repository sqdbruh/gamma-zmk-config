/*
 * Halt-on-fatal override.
 *
 * Default Zephyr behaviour on a fatal error is to log + reset, which
 * bounces straight back into the bootloader and gives no chance to
 * read the panic line over RTT or to inspect the faulting state with
 * a debugger. Override the handler with an infinite loop so the
 * panic dump finishes flushing and the CPU stays at a known PC.
 *
 * Compile-time gated by CONFIG_GAMMA_HALT_ON_FATAL so production
 * builds keep the default reset-on-panic behaviour.
 */

#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_GAMMA_HALT_ON_FATAL)

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf) {
    ARG_UNUSED(esf);
    LOG_ERR("=== GAMMA FATAL: reason=%u ===", reason);
    log_panic();
    /* Park forever so RTT keeps the panic frame and JLink halt sees a
     * deterministic PC inside this loop. The R7 frame pointer (or the
     * stacked PC at MSP+24) gives the actual fault site. */
    while (1) {
        __asm__ __volatile__("wfi");
    }
}

#endif
