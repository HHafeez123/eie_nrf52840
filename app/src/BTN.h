#ifndef BTN_H
#define BTN_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdbool.h>

/* We only need button 0 for the lesson */
typedef enum {
    BTN0 = 0,
} btn_id;

/* Button 0 is the board alias sw0 */
static const struct gpio_dt_spec btn0 =
    GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/* Simple init: configure button 0 as input */
static inline int BTN_init(void)
{
    int ret;

    if (!device_is_ready(btn0.port)) {
        return -1;
    }

    ret = gpio_pin_configure_dt(&btn0, GPIO_INPUT);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

/*
 * Debounced “press” check.
 * Returns true once per *real* press.
 * Internally it:
 *  - looks for a rising edge,
 *  - waits a short time,
 *  - checks again to filter out bouncing,
 *  - then latches a single “pressed” event.
 */
static inline bool BTN_check_clear_pressed(btn_id btn)
{
    (void)btn; /* only BTN0 is supported for now */

    static bool prev_level = false;
    static bool latched = false;

    bool level = gpio_pin_get_dt(&btn0);

    /* Rising edge detected */
    if (level && !prev_level) {
        /* Small debounce delay */
        k_msleep(10);
        level = gpio_pin_get_dt(&btn0);
        if (level) {
            latched = true;
        }
    }

    prev_level = level;

    if (latched) {
        latched = false;   /* clear the internal flag */
        return true;
    }

    return false;
}

#endif /* BTN_H */
