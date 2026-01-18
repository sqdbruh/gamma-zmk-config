#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#if IS_ENABLED(CONFIG_GAMMA_LED_TEST)

#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(STRIP_NODE, chain_length)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_NODE, chain_length)
#else
#error "LED strip chain-length missing"
#endif

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static const struct gpio_dt_spec led_enable =
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led_enable), gpios, {0});

static struct led_rgb leds_buffer[STRIP_NUM_PIXELS];

static int gamma_led_test_init(void) {
    if (!device_is_ready(strip)) {
        return 0;
    }

    if (device_is_ready(led_enable.port)) {
        gpio_pin_configure_dt(&led_enable, GPIO_OUTPUT_ACTIVE);
        k_msleep(10);
    }

    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        leds_buffer[i].r = 255;
        leds_buffer[i].g = 255;
        leds_buffer[i].b = 255;
    }

    (void)led_strip_update_rgb(strip, leds_buffer, STRIP_NUM_PIXELS);
    return 0;
}

SYS_INIT(gamma_led_test_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
