#pragma once

#include <atomic>
#include <cstdint>

#include "driver/gpio.h"

namespace bcd {

// Parametros de cada tarea de boton (se entregan por pvParameters).
struct ButtonConfig {
    const char*        name;
    gpio_num_t         pin;
    std::atomic<bool>* flag;  // flag que se levanta al detectar el flanco
};

// Boton con antirrebote por muestreo y deteccion de flanco de presion.
class Button {
public:
    Button(gpio_num_t pin, uint8_t debounceSamples);

    void init();
    bool update();  // llamar periodicamente; true SOLO en el flanco de presion

private:
    bool readRaw() const;

    gpio_num_t pin_;
    uint8_t    debounceSamples_;
    uint8_t    count_;
    bool       stablePressed_;
};

}  // namespace bcd
