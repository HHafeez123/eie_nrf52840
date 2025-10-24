/*
 * main.c
 */

 /*
 * main.c
 */

 #include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

/* ---- Button (BTN0) ---- */
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

/* ---- LEDs (led0..led3) ---- */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

static const struct gpio_dt_spec leds[4] = {
#if DT_NODE_HAS_STATUS(LED0_NODE, okay) && DT_NODE_HAS_PROP(LED0_NODE, gpios)
    GPIO_DT_SPEC_GET(LED0_NODE, gpios),
#else
#   error "led0 alias not found in devicetree"
#endif
#if DT_NODE_HAS_STATUS(LED1_NODE, okay) && DT_NODE_HAS_PROP(LED1_NODE, gpios)
    GPIO_DT_SPEC_GET(LED1_NODE, gpios),
#else
#   error "led1 alias not found in devicetree"
#endif
#if DT_NODE_HAS_STATUS(LED2_NODE, okay) && DT_NODE_HAS_PROP(LED2_NODE, gpios)
    GPIO_DT_SPEC_GET(LED2_NODE, gpios),
#else
#   error "led2 alias not found in devicetree"
#endif
#if DT_NODE_HAS_STATUS(LED3_NODE, okay) && DT_NODE_HAS_PROP(LED3_NODE, gpios)
    GPIO_DT_SPEC_GET(LED3_NODE, gpios),
#else
#   error "led3 alias not found in devicetree"
#endif
};

/* ---- State ---- */
static volatile uint8_t count4 = 0;               // 0..15
static struct gpio_callback button_cb;
static volatile uint32_t last_ms = 0;             // for simple debounce
#define DEBOUNCE_MS 150

/* Update the LEDs to show 'count4' in binary (LSB on led0). */
static inline void show_count_on_leds(uint8_t v)
{
    for (int i = 0; i < 4; ++i) {
        /* gpio_pin_set_dt respects active-low/active-high from DT flags */
        gpio_pin_set_dt(&leds[i], (v >> i) & 0x1);
    }
}

/* ISR: runs when BTN0 transitions to ACTIVE (pressed). */
static void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    /* Simple time-based debounce (safe to call in ISR) */
    uint32_t now = k_uptime_get_32();
    if (now - last_ms < DEBOUNCE_MS) {
        return;
    }
    last_ms = now;

    /* Increment and wrap at 16 */
    count4 = (uint8_t)((count4 + 1) & 0x0F);  // wraps 15->0 when incremented past 15 with mask
    if (count4 == 0) {
        /* If you prefer to show 0000 on wrap, nothing else to do.
           If you wanted “reset once reaches 16”, this is exactly that: we show 0000. */
    }
    show_count_on_leds(count4);
}

int main(void)
{
    int ret;

    /* ---- Check and configure button ---- */
    if (!gpio_is_ready_dt(&button)) {
        return 0;
    }
    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        return 0;
    }
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        return 0;
    }
    gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb);

    /* ---- Configure LEDs as outputs, start at 0000 ---- */
    for (int i = 0; i < 4; ++i) {
        if (!gpio_is_ready_dt(&leds[i])) {
            return 0;
        }
        ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            return 0;
        }
    }
    show_count_on_leds(count4);  // start at 0

    /* Nothing to do in the main loop — interrupts drive the counter */
    while (1) {
        k_msleep(1000);
    }
    return 0;
}