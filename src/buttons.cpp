#include "buttons.hpp"

#include "app_config.hpp"

namespace bcd {

Button::Button(gpio_num_t pin, uint8_t debounceSamples)
    : pin_(pin), debounceSamples_(debounceSamples), count_(0), stablePressed_(false) {}

void Button::init() {
    const bool activeLow = (config::kButtonPressedLevel == 0);

    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << pin_);
    io.mode         = GPIO_MODE_INPUT;
    io.pull_up_en   = activeLow ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io.pull_down_en = activeLow ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE;
    io.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&io);

    stablePressed_ = readRaw();  // evita un flanco falso al arrancar
}

bool Button::readRaw() const {
    return gpio_get_level(pin_) == config::kButtonPressedLevel;
}

bool Button::update() {
    const bool raw = readRaw();
    if (raw == stablePressed_) {
        count_ = 0;
        return false;
    }
    // La lectura es distinta al estado estable: debe mantenerse N muestras
    if (++count_ < debounceSamples_) {
        return false;
    }
    count_         = 0;
    stablePressed_ = raw;
    return stablePressed_;  // true solo en la transicion suelto -> presionado
}

}  // namespace bcd
