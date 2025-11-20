#ifndef BTN_H
#define BTN_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdbool.h>

/* Four buttons: 0..3 */
typedef enum {
    BTN0 = 0,  // sw0 -> BUTTON 1
    BTN1,      // sw1 -> BUTTON 2
    BTN2,      // sw2 -> BUTTON 3
    BTN3,      // sw3 -> BUTTON 4
    BTN_COUNT
} btn_id;

/* Map each BTN to a devicetree alias sw0..sw3 */
static const struct gpio_dt_spec btns[BTN_COUNT] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios),
};

/* Per-button debounce state */
static bool btn_prev_level[BTN_COUNT] = { false };
static bool btn_latched[BTN_COUNT]    = { false };

/* Configure all buttons as inputs */
static inline int BTN_init(void)
{
    for (int i = 0; i < BTN_COUNT; ++i) {
        if (!device_is_ready(btns[i].port)) {
            return -1;
        }
        int ret = gpio_pin_configure_dt(&btns[i], GPIO_INPUT);
        if (ret < 0) {
            return ret;
        }
        /* Initialize previous level */
        btn_prev_level[i] = gpio_pin_get_dt(&btns[i]) ? true : false;
        btn_latched[i] = false;
    }
    return 0;
}

/*
 * Debounced “press” check for a specific button.
 * Returns true once per real press.
 */
static inline bool BTN_check_clear_pressed(btn_id btn)
{
    if (btn < 0 || btn >= BTN_COUNT) {
        return false;
    }

    bool level = gpio_pin_get_dt(&btns[btn]);
    bool prev  = btn_prev_level[btn];

    /* Rising edge detected */
    if (level && !prev) {
        /* Small debounce delay */
        k_msleep(10);
        level = gpio_pin_get_dt(&btns[btn]);
        if (level) {
            btn_latched[btn] = true;
        }
    }

    btn_prev_level[btn] = level;

    if (btn_latched[btn]) {
        btn_latched[btn] = false;
        return true;
    }

    return false;
}

#endif /* BTN_H */

