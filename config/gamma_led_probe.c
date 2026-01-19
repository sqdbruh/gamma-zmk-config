#include <hal/nrf_gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#if IS_ENABLED(CONFIG_GAMMA_LED_PROBE)

#define GAMMA_PROBE_MARKER "GAMMA_PROBE_V1_2026_01_19"

#define PROBE0_PIN NRF_GPIO_PIN_MAP(0, 4)
#define PROBE1_PIN NRF_GPIO_PIN_MAP(0, 7)
#define LED_GATE_PIN NRF_GPIO_PIN_MAP(0, 6)

#if defined(CONFIG_GAMMA_LED_PROBE)
__attribute__((used))
static const volatile char gamma_probe_marker[] = GAMMA_PROBE_MARKER;

__attribute__((used, noinline))
static void gamma_probe_touch_marker(void) {
    volatile char c = gamma_probe_marker[0];
    (void)c;
}
#endif

static int gamma_led_probe_init(void) {
#if defined(CONFIG_GAMMA_LED_PROBE)
    gamma_probe_touch_marker();
#endif

    nrf_gpio_cfg_output(LED_GATE_PIN);
    nrf_gpio_pin_set(LED_GATE_PIN);

    nrf_gpio_cfg_output(PROBE0_PIN);
    nrf_gpio_cfg_output(PROBE1_PIN);
    nrf_gpio_pin_set(PROBE0_PIN);
    nrf_gpio_pin_set(PROBE1_PIN);
    return 0;
}

SYS_INIT(gamma_led_probe_init, PRE_KERNEL_1, 0);

#endif
