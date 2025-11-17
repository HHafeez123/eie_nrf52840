#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

// Define device-tree aliases
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

// Convert aliases into gpio_dt_spec structures
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

int main(void)
{
    int ret;

    // Array of LEDs in normal order (0,1,2,3)
    const struct gpio_dt_spec leds[4] = { led0, led1, led2, led3 };
    const int NUM_LEDS = 4;

    // Make sure all LEDs are ready
    for (int i = 0; i < NUM_LEDS; i++) {
        if (!device_is_ready(leds[i].port)) {
            return -1;
        }
    }

    // Configure all LEDs as outputs (start off)
    for (int i = 0; i < NUM_LEDS; i++) {
        ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
        if (ret < 0) return ret;
    }

    // State variables for Knight Rider pattern
    int index = 0;   // start at LED0
    int dir = 1;     // +1 means move forward, -1 means backward

    while (1) {

        // Turn all LEDs off
        for (int i = 0; i < NUM_LEDS; i++) {
            gpio_pin_set_dt(&leds[i], 0);
        }

        // Turn current LED on
        gpio_pin_set_dt(&leds[index], 1);

        // Timing between steps
        k_msleep(150);

        // Update index
        index += dir;

        // Check if we hit the ends, then reverse direction
        if (index == NUM_LEDS - 1) {
            dir = -1;   // reached LED3 → go backward
        }
        else if (index == 0) {
            dir = 1;    // reached LED0 → go forward
        }
    }

    return 0;
}
