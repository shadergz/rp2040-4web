#include <pico/stdlib.h>

static const uint8_t pinouts[] = {
    18, 19, 20
};

int main() {
    for (uint8_t port = 0; port < 3; port++) {
        gpio_init(pinouts[port]);
        gpio_set_dir(pinouts[port], GPIO_OUT);
    }
    volatile const uint loop = 1;
    volatile uint high = 0;

    while (loop) {
        const uint8_t pin = pinouts[high];
        ++high;

        gpio_put(pin, 1);
        sleep_ms(250);
        gpio_put(pin, 0);
        sleep_ms(250);

        if (high == 3)
            high = 0;
    }
}
