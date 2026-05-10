#ifndef CONFIG_BOARD_GAMMA_DONGLE
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>
#include <zmk/usb.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/battery.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zmk/workqueue.h>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

static const struct gpio_dt_spec led188_1 = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led188_1), gpios, {0});
static const struct gpio_dt_spec led188_2 = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led188_2), gpios, {0});
static const struct gpio_dt_spec led188_3 = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led188_3), gpios, {0});
static const struct gpio_dt_spec led188_4 = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led188_4), gpios, {0});
static const struct gpio_dt_spec led188_5 = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led188_5), gpios, {0});
static const struct gpio_dt_spec charging_status =
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(chrg_status), gpios, {0});

static const struct gpio_dt_spec led188[] = {led188_1, led188_2, led188_3, led188_4, led188_5};
// uint32_t ledCache[ArrayCount(led188)];

#define HIGH GPIO_OUTPUT_ACTIVE
#define LOW GPIO_OUTPUT_INACTIVE
#define NO GPIO_DISCONNECTED

#include "digits.h"

static void set_pin(uint32_t index, uint32_t val) {
    // uint32_t cached = ledCache[index];
    // if (cached != val) {

    // bool configure = (cached == NO && (val == HIGH || val == LOW)) ||
    // ((cached == HIGH || cached == LOW) && val == NO);

    //         if (configure) {
    struct gpio_dt_spec pin = led188[index];
    gpio_pin_configure_dt(&pin, val);
    // } else {
    //     gpio_pin_set_dt(&pin, val == HIGH ? 1 : 0);
    // }

    // ledCache[index] = val;
    // }
}

static void disable_all_leds() {
    for (int ledPinIndex = 0; ledPinIndex < ArrayCount(led188); ledPinIndex++) {
        set_pin(ledPinIndex, NO);
    }
}

int currentSegment;
int wait;

static void display_digit(uint16_t digit) {
    if (wait) {
        wait--;
        return;
    }

    bool isSegmentSet = false;
    struct segment_info seg;
    // do {
    uint16_t mask = (1 << currentSegment);
    isSegmentSet = (mask & digit) == mask;
    seg = segments[currentSegment];
    currentSegment++;
    if (currentSegment > ArrayCount(segments)) {
        currentSegment = 0;
    }

    // } while (!isSegmentSet);

    if (isSegmentSet) {
        for (int ledPinIndex = 0; ledPinIndex < ArrayCount(led188); ledPinIndex++) {
            uint32_t val = NO;
            if (ledPinIndex == seg.high - 1) {
                val = HIGH;
            } else if (ledPinIndex == seg.low - 1) {
                val = LOW;
            }
            struct gpio_dt_spec pin = led188[ledPinIndex];
            gpio_pin_configure_dt(&pin, val);
        }
        wait = 1;
    } else {
        for (int ledPinIndex = 0; ledPinIndex < ArrayCount(led188); ledPinIndex++) {
            gpio_pin_configure_dt(&led188[ledPinIndex], NO);
        }
        wait = 1;
    }
}

volatile bool showBattery;

void stop_timer();

static void seg_display_tick() {
    bool charged = gpio_pin_get_dt(&charging_status);
    bool isConnected = zmk_split_bt_peripheral_is_connected();
    bool isPowered = zmk_usb_is_powered();

    if (showBattery) {
        uint8_t charge = zmk_battery_state_of_charge();
        display_digit(digits188[charge]);
    } else {
        if (isPowered) {
            if (charged) {
                display_digit(digits188[100]);
            } else {
                display_digit(digit_xCh);
            }
        } else {
            if (isConnected) {
                // NOTE(sqd): Skip
                disable_all_leds();
                stop_timer();
            } else {
                display_digit(digit_xbt);
            }
        }
    }
}
K_WORK_DEFINE(seg_display_tick_work, seg_display_tick);
static void seg_display_tick_handler(struct k_timer *timer) {
    seg_display_tick();
    // k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &seg_display_tick_work);
}
K_TIMER_DEFINE(seg_display_tick_timer, seg_display_tick_handler, NULL);
volatile bool isTimerOn;

void stop_timer() {
    if (isTimerOn) {
        k_timer_stop(&seg_display_tick_timer);
        isTimerOn = false;
    }
}

static void start_timer() {
    if (!isTimerOn) {
        k_timer_start(&seg_display_tick_timer, K_NO_WAIT, K_USEC(350));
        isTimerOn = true;
    }
}

static int seg_display_init(void) {
    gpio_pin_configure_dt(&charging_status, GPIO_INPUT);
    showBattery = false;
    start_timer();

    return 0;
}

void show_battery() {
    showBattery = true;
    start_timer();
}
void hide_battery() { showBattery = false; }

static int seg_display_event_listener(const zmk_event_t *eh) {

    if (as_zmk_battery_state_changed(eh)) {
        start_timer();
    }

    if (as_zmk_usb_conn_state_changed(eh)) {
        start_timer();
    }

    if (as_zmk_activity_state_changed(eh)) {
    }

    return 0;
}

/*static int led_init(const struct device *_arg){};*/
ZMK_LISTENER(seg_display, seg_display_event_listener);

ZMK_SUBSCRIPTION(seg_display, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(seg_display, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(seg_display, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

SYS_INIT(seg_display_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
