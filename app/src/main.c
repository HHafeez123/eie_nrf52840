/*
 * Challenge Part 2:
 *
 * - On startup: LED3 ON for 3 seconds (boot window)
 *   - If BTN3 is pressed during this time -> enter PASSWORD ENTRY mode.
 * - PASSWORD ENTRY mode:
 *   - User presses BTN0/BTN1/BTN2 in any combination.
 *   - Each button sets a bit in a mask (like part 1).
 *   - When BTN3 is pressed again, that mask becomes the new password.
 *   - Then we go to LOCKED state.
 * - LOCKED state:
 *   - Same behavior as Part 1:
 *     - BTN0/BTN1/BTN2 set bits in entered_mask.
 *     - BTN3 checks the password (Correct!/Incorrect!).
 *     - LED0 ON = locked, LED0 OFF = waiting.
 * - WAITING state:
 *   - Any button press resets back to LOCKED.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "BTN.h"   // Your updated BTN.h with BTN0..BTN3

#define SLEEP_MS 5
#define BOOT_WINDOW_MS 3000  // 3 seconds

/* LED0 and LED3 from devicetree */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);

/* Button bits for the mask */
#define BTN0_BIT 0x1  // BTN0 (BUTTON 1)
#define BTN1_BIT 0x2  // BTN1 (BUTTON 2)
#define BTN2_BIT 0x4  // BTN2 (BUTTON 3)

/* States */
typedef enum {
    STATE_BOOT_WINDOW = 0,
    STATE_ENTRY,     // password programming
    STATE_LOCKED,    // like part 1
    STATE_WAITING    // after checking password
} system_state_t;

static system_state_t state;

/* Password and entered masks */
static uint8_t password_mask = (BTN0_BIT | BTN2_BIT);  // default: BTN0 + BTN2 (5)
static uint8_t entered_mask = 0;   // used in LOCKED state
static uint8_t entry_mask   = 0;   // used in ENTRY mode (programming)

/* Boot window deadline (ms since boot) */
static int64_t boot_deadline_ms = 0;

/* ---- LED helpers ---- */

static int leds_init(void)
{
    if (!device_is_ready(led0.port) || !device_is_ready(led3.port)) {
        return -1;
    }

    /* Configure as outputs, start OFF */
    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);

    return 0;
}

static void led0_on(void)  { gpio_pin_set_dt(&led0, 1); }
static void led0_off(void) { gpio_pin_set_dt(&led0, 0); }
static void led3_on(void)  { gpio_pin_set_dt(&led3, 1); }
static void led3_off(void) { gpio_pin_set_dt(&led3, 0); }

/* ---- State helpers ---- */

static void go_locked(void)
{
    state = STATE_LOCKED;
    entered_mask = 0;
    led0_on();   // locked indicator
    led3_off();  // LED3 not used in locked mode
    printk("LOCKED. Password mask = %u. Use BTN0..BTN2 then BTN3.\n", password_mask);
}

static void go_waiting(void)
{
    state = STATE_WAITING;
    led0_off();   // off while waiting
    printk("WAITING. Press any button to reset to LOCKED.\n");
}

static void go_entry(void)
{
    state = STATE_ENTRY;
    entry_mask = 0;
    // Keep LED3 ON to indicate we are in entry mode
    led0_off();   // not locked yet
    printk("ENTRY MODE. Press BTN0/BTN1/BTN2 to set password, BTN3 to save.\n");
}

/* ---- main ---- */

int main(void)
{
    if (BTN_init() < 0) {
        return 0;
    }
    if (leds_init() < 0) {
        return 0;
    }

    /* Start in boot window: LED3 ON for 3 seconds */
    state = STATE_BOOT_WINDOW;
    boot_deadline_ms = k_uptime_get() + BOOT_WINDOW_MS;
    led3_on();
    led0_off();

    printk("BOOT WINDOW: LED3 ON for 3 seconds.\n");
    printk("Press BTN3 now to enter password ENTRY mode.\n");
    printk("If you do nothing, default password (BTN0+BTN2) is used.\n");

    while (1) {

        if (state == STATE_BOOT_WINDOW) {
            int64_t now = k_uptime_get();

            /* If BTN3 is pressed: enter password programming mode */
            if (BTN_check_clear_pressed(BTN3)) {
                go_entry();
            }
            /* If 3 seconds have passed and we are still in boot window: go locked */
            else if (now >= boot_deadline_ms) {
                led3_off();
                go_locked();
            }
        }
        else if (state == STATE_ENTRY) {
            /* Build password mask using BTN0/BTN1/BTN2 */
            if (BTN_check_clear_pressed(BTN0)) {
                entry_mask |= BTN0_BIT;
                printk("ENTRY: BTN0 pressed. entry_mask = %u\n", entry_mask);
            }
            if (BTN_check_clear_pressed(BTN1)) {
                entry_mask |= BTN1_BIT;
                printk("ENTRY: BTN1 pressed. entry_mask = %u\n", entry_mask);
            }
            if (BTN_check_clear_pressed(BTN2)) {
                entry_mask |= BTN2_BIT;
                printk("ENTRY: BTN2 pressed. entry_mask = %u\n", entry_mask);
            }

            /* BTN3 saves the password and goes to locked state */
            if (BTN_check_clear_pressed(BTN3)) {
                if (entry_mask != 0) {
                    password_mask = entry_mask;
                    printk("ENTRY: Password saved! New password mask = %u\n", password_mask);
                } else {
                    printk("ENTRY: No buttons pressed, keeping old password (%u).\n", password_mask);
                }
                led3_off();
                go_locked();
            }
        }
        else if (state == STATE_LOCKED) {
            /* Part 1 behavior: BTN0..BTN2 build entered_mask */
            if (BTN_check_clear_pressed(BTN0)) {
                entered_mask |= BTN0_BIT;
                printk("LOCKED: BTN0 pressed. entered_mask = %u\n", entered_mask);
            }
            if (BTN_check_clear_pressed(BTN1)) {
                entered_mask |= BTN1_BIT;
                printk("LOCKED: BTN1 pressed. entered_mask = %u\n", entered_mask);
            }
            if (BTN_check_clear_pressed(BTN2)) {
                entered_mask |= BTN2_BIT;
                printk("LOCKED: BTN2 pressed. entered_mask = %u\n", entered_mask);
            }

            /* BTN3 checks password */
            if (BTN_check_clear_pressed(BTN3)) {
                printk("LOCKED: ENTER (BTN3). entered_mask = %u, password_mask = %u\n",
                       entered_mask, password_mask);
                if (entered_mask == password_mask) {
                    printk("Correct!\n");
                } else {
                    printk("Incorrect!\n");
                }
                go_waiting();
            }
        }
        else if (state == STATE_WAITING) {
            /* Any button resets back to locked */
            if (BTN_check_clear_pressed(BTN0) ||
                BTN_check_clear_pressed(BTN1) ||
                BTN_check_clear_pressed(BTN2) ||
                BTN_check_clear_pressed(BTN3)) {

                printk("RESET: Returning to LOCKED state.\n");
                go_locked();
            }
        }

        k_msleep(SLEEP_MS);
    }

    return 0;
}
