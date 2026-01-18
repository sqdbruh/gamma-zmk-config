#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#if IS_ENABLED(CONFIG_GAMMA_LED_PROBE)

#define PROBE0_PIN 24
#define PROBE1_PIN 22
#define PROBE2_PIN 0

static const struct device *const gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *const gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
static const struct gpio_dt_spec led_enable =
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led_enable), gpios, {0});

static bool probe_state;

static int gamma_led_probe_init(void) {
    if (!device_is_ready(gpio0)) {
        return 0;
    }

    if (device_is_ready(led_enable.port)) {
        gpio_pin_configure_dt(&led_enable, GPIO_OUTPUT_ACTIVE);
        k_msleep(10);
    }

    gpio_pin_configure(gpio0, PROBE0_PIN, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(gpio0, PROBE1_PIN, GPIO_OUTPUT_INACTIVE);
    if (device_is_ready(gpio1)) {
        gpio_pin_configure(gpio1, PROBE2_PIN, GPIO_OUTPUT_INACTIVE);
    }
    while (true) {
        probe_state = !probe_state;
        (void)gpio_pin_set(gpio0, PROBE0_PIN, probe_state);
        (void)gpio_pin_set(gpio0, PROBE1_PIN, probe_state);
        if (device_is_ready(gpio1)) {
            (void)gpio_pin_set(gpio1, PROBE2_PIN, probe_state);
        }
        k_busy_wait(5000000);
    }

    return 0;
}

SYS_INIT(gamma_led_probe_init, PRE_KERNEL_1, 0);

#endif
