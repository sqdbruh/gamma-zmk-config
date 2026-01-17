#ifndef CONFIG_BOARD_GAMMA_DONGLE
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/events/endpoint_changed.h>
#include <zmk/endpoints.h>
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
#include <zephyr/drivers/led_strip.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zmk/workqueue.h>

#define CURRENT_BRIGHTNESS 255

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
static float remap(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
    return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

static const struct device *spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi3));

static const struct gpio_dt_spec charging_status =
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(chrg_status), gpios, {0});

static const struct gpio_dt_spec led_enable =
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led_enable), gpios, {0});

#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

static struct led_rgb leds_buffer[STRIP_NUM_PIXELS];
static const uint8_t led_map[STRIP_NUM_PIXELS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

static void buffer_single_led_color(int logical_idx, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t phys = led_map[logical_idx];
    leds_buffer[phys].r = r;
    leds_buffer[phys].g = g;
    leds_buffer[phys].b = b;
}

static void buffer_all_leds_color(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        buffer_single_led_color(i, r, g, b);
    }
}

static void update_leds() {
    int rc = led_strip_update_rgb(strip, leds_buffer, STRIP_NUM_PIXELS);
    if (rc) {
        LOG_ERR("Couldn't update strip: %d", rc);
    }
}

static void update_leds_color(uint8_t r, uint8_t g, uint8_t b) {
    buffer_all_leds_color(r, g, b);
    update_leds();
}

static void soc_to_led_buffer(uint8_t soc, struct led_rgb *buffer) {
    int num_leds = STRIP_NUM_PIXELS;

    int full_leds = (soc * num_leds) / 100;
    if (full_leds == 0) {
        full_leds = 1; // хотя бы один LED
    }
    if (soc > 90) {
        full_leds = num_leds;
    }

    const uint8_t dim_white = 4;

    for (int i = 0; i < num_leds; i++) {
        uint8_t r = 0, g = 0, b = 0;

        if (i < full_leds) {
            // Заряженные сегменты
            if (i == 0) {
                r = CURRENT_BRIGHTNESS; // красный
            } else if (i == 1) {
                r = CURRENT_BRIGHTNESS; // жёлтый
                g = (uint8_t)(CURRENT_BRIGHTNESS * 0.5f);
            } else {
                g = CURRENT_BRIGHTNESS; // зелёный
            }
        } else {
            // Незаряженные сегменты — тусклый белый
            r = dim_white;
            g = dim_white;
            b = dim_white;
        }

        buffer[i].r = r;
        buffer[i].g = g;
        buffer[i].b = b;
    }
}

// #define CHECK_BATTERY_EVERY 10000
// int tickToCheckBattery = CHECK_BATTERY_EVERY;
volatile bool showBattery;
volatile bool showBatteryDisplay;
volatile bool isPowered;
volatile bool isConnected;
volatile bool isCharging;

struct gamma_led_state {
    void (*onTick)(struct gamma_led_state *);
    void (*onComplete)();
    uint32_t currentTick;
    uint32_t durationInTicks;
    float t01;
};

#define QUEUE_SIZE (128)
struct gamma_led_queue {
    struct gamma_led_state queue[QUEUE_SIZE];
    int head;
    int tail;
    int count;
};

volatile struct gamma_led_queue led_queue;

static void init_queue(volatile struct gamma_led_queue *q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

static bool is_queue_full(volatile struct gamma_led_queue *q) { return q->count == QUEUE_SIZE; }

static bool is_queue_empty(volatile struct gamma_led_queue *q) { return q->count == 0; }

static void gamma_tick(struct k_work *work);
K_WORK_DEFINE(gamma_tick_work, gamma_tick);

static void gamma_tick_handler(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &gamma_tick_work);
}
K_TIMER_DEFINE(gamma_tick_timer, gamma_tick_handler, NULL);

static void enable_led_power(bool enable) {
    if (enable) {
        // First enable the GPIO
        gpio_pin_set_dt(&led_enable, true);
        k_msleep(10); // Give the LED power time to stabilize

        if (device_is_ready(spi_dev)) {
            int ret = pm_device_action_run(spi_dev, PM_DEVICE_ACTION_RESUME);
            if (ret < 0) {
                LOG_ERR("Failed to resume SPI3: %d", ret);
                return;
            }
            k_msleep(10); // Give SPI time to initialize
            LOG_INF("SPI3 resumed successfully");
        }
    } else {
        if (device_is_ready(spi_dev)) {
            int ret = pm_device_action_run(spi_dev, PM_DEVICE_ACTION_SUSPEND);
            if (ret < 0) {
                LOG_ERR("Failed to suspend SPI3: %d", ret);
            }
        }
        gpio_pin_set_dt(&led_enable, false);
    }
}

static bool enqueue(volatile struct gamma_led_queue *q, struct gamma_led_state value) {
    if (is_queue_full(q)) {
        return false;
    }
    q->queue[q->tail] = value;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    q->count++;
    LOG_INF("Enqueue. Queue count: %i ", q->count);

    if (k_timer_remaining_get(&gamma_tick_timer) == 0) {
        enable_led_power(true);
        k_timer_start(&gamma_tick_timer, K_NO_WAIT, K_MSEC(1));
        LOG_INF("Start timer and led");
    }

    return true;
}

static bool dequeue(volatile struct gamma_led_queue *q, struct gamma_led_state *value) {
    if (is_queue_empty(q)) {
        return false;
    }
    *value = q->queue[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->count--;
    LOG_INF("Dequeue. Queue count: %i ", q->count);
    return true;
}

static struct gamma_led_state currentState;

void startup_tick(struct gamma_led_state *state) {
    uint8_t soc = zmk_battery_state_of_charge();
    int num_leds = STRIP_NUM_PIXELS;

    // Подготовим буфер с цветами всех сегментов (заряженные + тусклый белый для пустых)
    struct led_rgb temp_buffer[STRIP_NUM_PIXELS];
    soc_to_led_buffer(soc, temp_buffer);

    // Рассчёт fade-in/out
    float fade_factor = 1.0f;
    uint32_t fade_in_ticks = (state->durationInTicks * 30) / 100;
    uint32_t fade_out_start_tick = (state->durationInTicks * 80) / 100;

    if (state->currentTick <= fade_in_ticks) {
        fade_factor = (float)state->currentTick / fade_in_ticks;
    } else if (state->currentTick >= fade_out_start_tick) {
        uint32_t fade_ticks = state->currentTick - fade_out_start_tick;
        fade_factor = 1.0f - ((float)fade_ticks / (state->durationInTicks - fade_out_start_tick));
    }

    // Wave-эффект (опционально—можно убрать, тогда просто пойдёт равномерный fade)
    uint32_t total_ticks = state->durationInTicks;
    uint32_t wave_ticks = (total_ticks * 10) / 100;
    uint32_t delay_per_led = wave_ticks / num_leds;
    uint32_t start_tick, end_tick;

    for (int i = 0; i < num_leds; i++) {
        // базовые цвета из буфера + fade_factor
        uint8_t r_base = (uint8_t)(temp_buffer[i].r * fade_factor);
        uint8_t g_base = (uint8_t)(temp_buffer[i].g * fade_factor);
        uint8_t b_base = (uint8_t)(temp_buffer[i].b * fade_factor);

        if (state->currentTick <= wave_ticks) {
            start_tick = i * delay_per_led;
            end_tick = start_tick + delay_per_led;
            if (state->currentTick < start_tick) {
                // ещё не дошли до этого LED
                buffer_single_led_color(i, 0, 0, 0);
                continue;
            } else if (state->currentTick < end_tick) {
                // плавный quadratic fade-in для этого LED
                uint32_t elapsed = state->currentTick - start_tick;
                uint32_t prog = (elapsed * 255) / delay_per_led;
                uint32_t qfade = (prog * prog) / 255;
                buffer_single_led_color(i, (r_base * qfade) / 255, (g_base * qfade) / 255,
                                        (b_base * qfade) / 255);
                continue;
            }
            // иначе уже фазе полного яркого
        }

        // В все остальные моменты просто показываем цвет
        buffer_single_led_color(i, r_base, g_base, b_base);
    }

    update_leds();
}

void ble_disconnected_tick(struct gamma_led_state *state) {
    float brightness = 0.0f;
    if (state->t01 < 0.5f) {
        brightness = remap(state->t01, 0.0f, 0.5f, 0.0f, 1.0f);
    } else {
        brightness = remap(state->t01, 0.5f, 1.0f, 1.0f, 0.0f);
    }
    int v = brightness * 255;
    buffer_all_leds_color(0, 0, v);

    update_leds();
}

void battery_charging_tick() {
    static float trail[STRIP_NUM_PIXELS] = {0.0f};
    static int forward_pos = 0;
    static bool waiting = false;
    static uint32_t tick = 0;
    static int pause_ticks = 0;

    const int num_leds = STRIP_NUM_PIXELS;
    const int speed = 8;             // head moves every 3 calls
    const float fade_factor = 0.85f; // trail fade multiplier
    const float min_thresh = 0.01f;  // cutoff for zero
    const int pause_duration = 20;   // ticks to wait after full fade

    tick++;

    // 1) Fade existing trail
    for (int i = 0; i < num_leds; i++) {
        trail[i] *= fade_factor;
        if (trail[i] < min_thresh) {
            trail[i] = 0.0f;
        }
    }

    if (!waiting) {
        // 2) Move head
        if (tick % speed == 0) {
            forward_pos++;
            if (forward_pos >= num_leds) {
                // Reached top—enter waiting state
                waiting = true;
            }
        }
        // 3) Light up head if still in motion
        if (forward_pos < num_leds) {
            int head = forward_pos; // bottom-to-top
            trail[head] = 1.0f;
        }
    } else {
        // 4) Check if any trail remains
        bool any_left = false;
        for (int i = 0; i < num_leds; i++) {
            if (trail[i] > 0.0f) {
                any_left = true;
                break;
            }
        }
        if (!any_left) {
            // 5) Once fully faded, wait additional ticks
            if (pause_ticks < pause_duration) {
                pause_ticks++;
            } else {
                // Reset everything and restart
                waiting = false;
                forward_pos = 0;
                tick = 0;
                pause_ticks = 0;
            }
        }
    }

    // 6) Render LED strip
    for (int i = 0; i < num_leds; i++) {
        uint8_t b = (uint8_t)(trail[i] * 255);
        buffer_single_led_color(i, 0, b, 0);
    }
    update_leds();
}

void battery_fully_charged_tick() {
    // Показываем все светодиоды зеленым цветом при полной зарядке
    buffer_all_leds_color(0, CURRENT_BRIGHTNESS, 0);
    update_leds();
}

static bool is_charging(void) {
    int v = gpio_pin_get_dt(&charging_status); // с ACTIVE_LOW это уже инвертировано
    return v == 1; // 1 == «активно», т.е. заряд идёт
}
void ble_connected_tick(struct gamma_led_state *state) {
    float brightness = 0.0f;
    if (state->t01 < 0.8f) {
        brightness = remap(state->t01, 0.0f, 0.8f, 0.0f, 1.0f);
    } else {
        brightness = remap(state->t01, 0.8f, 1.0f, 1.0f, 0.0f);
    }
    int v = 255 * brightness;
    buffer_all_leds_color(0, 0, v);

    update_leds();
}

static void gamma_tick(struct k_work *work) {
    if (((currentState.currentTick < currentState.durationInTicks) && currentState.onTick) ||
        dequeue(&led_queue, &currentState)) {
        if (currentState.onTick) {
            currentState.t01 = (float)currentState.currentTick / currentState.durationInTicks;
            currentState.onTick(&currentState);
            currentState.currentTick++;
            if (currentState.currentTick == currentState.durationInTicks) {
                if (currentState.onComplete) {
                    currentState.onComplete();
                }
            }
        }
    } else if (isPowered) {
        enable_led_power(true);
        // Проверяем статус зарядки
        bool charging = is_charging();
        if (charging != isCharging) {
            isCharging = charging;
            LOG_INF("Charging status changed: %s", charging ? "charging" : "not charging");
        }
        if (isPowered) {
            if (isCharging) {
                battery_charging_tick();
            } else {
                battery_fully_charged_tick(); 
                // TODO(sqd): This might also indicate that no battery plugged in
            }
        }
    } else {
        update_leds_color(0, 0, 0);
        k_timer_stop(&gamma_tick_timer);
        enable_led_power(false);
        LOG_INF("Disable timer and led");
    }
}

static void show_ble_connected_animation() {
    LOG_INF("Show ble conn animation");
    struct gamma_led_state state = {};
    state.onTick = ble_connected_tick;
    state.onComplete = 0;
    state.currentTick = 0;
    state.durationInTicks = 300;
    enqueue(&led_queue, state);
}

static void show_ble_disconnected_animation();
static void check_for_connection() {
    if (isConnected) {
        show_ble_connected_animation();
    } else {
        show_ble_disconnected_animation();
    }
}

static void show_ble_disconnected_animation() {
    if (!isConnected) {
        LOG_INF("Show ble disconn animation");
        struct gamma_led_state state = {};
        state.onTick = ble_disconnected_tick;
        state.onComplete = check_for_connection;
        state.currentTick = 0;
        state.durationInTicks = 2500;
        enqueue(&led_queue, state);
    }
}

void show_startup_animation() {
    LOG_INF("Show startup");
    struct gamma_led_state state = {};
    state.onTick = startup_tick;
    state.onComplete = check_for_connection;
    state.currentTick = 0;
    state.durationInTicks = 2000;
    enqueue(&led_queue, state);
}

static void gamma_init_delayed() {
    k_msleep(10);
    LOG_INF("Init delayed");
    // NOTE(sqd): This will prevent first led tick from freezing for a while
    // (ZMK does something heavy on startup?)

    LOG_INF("Init queue");
    init_queue(&led_queue);
    show_startup_animation();

    LOG_INF("WS2812 chain length detected: %d pixels", STRIP_NUM_PIXELS);
}

K_WORK_DEFINE(gamma_init_delayed_work, gamma_init_delayed);

static int gamma_init(void) {
    LOG_INF("Gamma init");
    showBattery = false;
    isCharging = false;

    if (device_is_ready(strip)) {
        LOG_INF("Found LED strip device %s", strip->name);
    } else {
        LOG_ERR("LED strip device %s is not ready", strip->name);
    }

    int ret = gpio_pin_configure_dt(&led_enable, GPIO_OUTPUT);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
    }

    enable_led_power(false);

    if (!device_is_ready(charging_status.port)) {
        printk("Error: Charging status GPIO device not ready.\n");
    }

    ret = gpio_pin_configure_dt(&charging_status, GPIO_INPUT);
    if (ret < 0) {
        printk("Error: Failed to configure charging status GPIO (%d).\n", ret);
    }

    k_work_submit_to_queue(zmk_workqueue_lowprio_work_q(), &gamma_init_delayed_work);

    return 0;
}

static float calc_fade_in_factor(struct gamma_led_state *state) {
    float factor = (float)state->currentTick / (float)state->durationInTicks;
    return factor > 1.0f ? 1.0f : factor;
}

static float calc_fade_out_factor(struct gamma_led_state *state) {
    float factor = 1.0f - (float)state->currentTick / (float)state->durationInTicks;
    return factor < 0.0f ? 0.0f : factor;
}

static void show_battery_fade_in_tick(struct gamma_led_state *state) {
    uint8_t soc = zmk_battery_state_of_charge();
    struct led_rgb temp_buffer[STRIP_NUM_PIXELS];
    soc_to_led_buffer(soc, temp_buffer);

    float fade_factor = calc_fade_in_factor(state);
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        buffer_single_led_color(i, (uint8_t)(temp_buffer[i].r * fade_factor),
                                (uint8_t)(temp_buffer[i].g * fade_factor),
                                (uint8_t)(temp_buffer[i].b * fade_factor));
    }
    update_leds();
}

static void show_battery_fade_out_tick(struct gamma_led_state *state) {
    uint8_t soc = zmk_battery_state_of_charge();
    struct led_rgb temp_buffer[STRIP_NUM_PIXELS];
    soc_to_led_buffer(soc, temp_buffer);

    float fade_factor = calc_fade_out_factor(state);
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        buffer_single_led_color(i, (uint8_t)(temp_buffer[i].r * fade_factor),
                                (uint8_t)(temp_buffer[i].g * fade_factor),
                                (uint8_t)(temp_buffer[i].b * fade_factor));
    }
    update_leds();
}

static void show_battery_static_tick() {
    uint8_t soc = zmk_battery_state_of_charge();
    struct led_rgb temp_buffer[STRIP_NUM_PIXELS];
    soc_to_led_buffer(soc, temp_buffer);
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        buffer_single_led_color(i, (uint8_t)(temp_buffer[i].r), (uint8_t)(temp_buffer[i].g),
                                (uint8_t)(temp_buffer[i].b));
    }

    update_leds();
}

static void check_show_battery_animation();

static void start_show_battery_animation() {
    struct gamma_led_state state = {};
    state.onTick = show_battery_fade_in_tick;
    state.currentTick = 0;
    state.onComplete = check_show_battery_animation;
    state.durationInTicks = 100;
    enqueue(&led_queue, state);
}

static void keep_show_battery_animation() {
    struct gamma_led_state state = {};
    state.onTick = show_battery_static_tick;
    state.currentTick = 0;
    state.onComplete = check_show_battery_animation;
    state.durationInTicks = 1;
    enqueue(&led_queue, state);
}

static void on_hide_battery() { showBatteryDisplay = false; }

static void hide_show_battery_animation() {
    struct gamma_led_state state = {};
    state.onTick = show_battery_fade_out_tick;
    state.currentTick = 0;
    state.onComplete = on_hide_battery;
    state.durationInTicks = 100;
    enqueue(&led_queue, state);
}

static void check_show_battery_animation() {
    if (showBattery) {
        keep_show_battery_animation();
    } else {
        hide_show_battery_animation();
    }
}

void show_battery() {
    if (!isPowered) {
        showBattery = true;
        if (!showBatteryDisplay) {
            showBatteryDisplay = true;
            start_show_battery_animation();
        }
    }
}
void hide_battery() { showBattery = false; }

static int gamma_event_listener(const zmk_event_t *eh) {
    if (as_zmk_split_peripheral_status_changed(eh)) {
        bool newConnected = zmk_split_bt_peripheral_is_connected();
        LOG_INF("BLE EVENT %i ", isConnected);
        if (newConnected != isConnected) {
            isConnected = newConnected;
            if (isConnected) {
                show_ble_connected_animation();
            } else {
                show_ble_disconnected_animation();
            }
        }
    }
    if (as_zmk_usb_conn_state_changed(eh)) {
        LOG_INF("USB CONN EVENT");
        isPowered = zmk_usb_is_powered();

        if (isPowered) {
            enable_led_power(true);
            k_timer_start(&gamma_tick_timer, K_NO_WAIT, K_MSEC(1));
        }
    }

    return 0;
}

ZMK_LISTENER(gamma_board, gamma_event_listener);

ZMK_SUBSCRIPTION(gamma_board, zmk_activity_state_changed);
ZMK_SUBSCRIPTION(gamma_board, zmk_battery_state_changed);
ZMK_SUBSCRIPTION(gamma_board, zmk_split_peripheral_status_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(gamma_board, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

// TODO(sqd): Non-dongle mode with animations and stuff
#if false
static int gamma_ble_event_listener(const zmk_event_t *eh) {
    LOG_INF("BLE EVENT");

    return 0;
}
ZMK_LISTENER(gamma_ble_listener, gamma_ble_event_listener);
ZMK_SUBSCRIPTION(gamma_ble_listener, zmk_ble_active_profile_changed);
ZMK_SUBSCRIPTION(gamma_ble_listener, zmk_endpoint_changed);

#endif

SYS_INIT(gamma_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
