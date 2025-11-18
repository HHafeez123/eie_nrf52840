#ifndef LED_H
#define LED_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* We only need LED 0 for the lesson */
typedef enum {
    LED0 = 0,
} led_id;

typedef enum {
    LED_OFF = 0,
    LED_ON  = 1,
} led_state;

/* LED0 alias from the board devicetree */
static const struct gpio_dt_spec led0 =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Configure LED0 as an output (starts off) */
static inline int LED_init(void)
{
    int ret;

    if (!device_is_ready(led0.port)) {
        return -1;
    }

    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

/* Toggle LED0 */
static inline int LED_toggle(led_id led)
{
    (void)led; /* only LED0 used for now */
    gpio_pin_toggle_dt(&led0);
    return 0;
}

/* Explicitly set LED0 on/off if you ever need it */
static inline int LED_set(led_id led, led_state state)
{
    (void)led;
    gpio_pin_set_dt(&led0, state == LED_ON ? 1 : 0);
    return 0;
}

#endif /* LED_H */
