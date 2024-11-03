#include "pico/stdlib.h"

int main() {
    // Initialize the LED pin (usually GPIO 25 for the onboard LED)
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        // Turn the LED on
        gpio_put(LED_PIN, 1);
        sleep_ms(500); // Delay for 500 milliseconds

        // Turn the LED off
        gpio_put(LED_PIN, 0);
        sleep_ms(500); // Delay for 500 milliseconds
    }
}
