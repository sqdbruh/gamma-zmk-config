#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#if IS_ENABLED(CONFIG_GAMMA_LED_PROBE)

#define PROBE0_PIN 4
#define PROBE1_PIN 7

static const struct device *const gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct gpio_dt_spec led_enable =
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led_enable), gpios, {0});

static int gamma_led_probe_init(void) {
    if (!device_is_ready(gpio0)) {
        return 0;
    }

    if (device_is_ready(led_enable.port)) {
        gpio_pin_configure_dt(&led_enable, GPIO_OUTPUT_ACTIVE);
        k_msleep(10);
    }

    gpio_pin_configure(gpio0, PROBE0_PIN, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure(gpio0, PROBE1_PIN, GPIO_OUTPUT_ACTIVE);
    return 0;
}

SYS_INIT(gamma_led_probe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
