#include <pico/stdlib.h>

int main() {
    const uint LED_PIN = 14;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    volatile const uint loop = 1;

    while (loop) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}