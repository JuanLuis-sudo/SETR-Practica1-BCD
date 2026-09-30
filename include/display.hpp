#pragma once

#include <cstdint>

#include "driver/gpio.h"

namespace bcd {

// Display de 7 segmentos de un digito (catodo comun).
// Las lineas a..g son compartidas entre displays (multiplexado) y cada
// display tiene su propia linea de habilitacion.
class SevenSegmentDisplay {
public:
    SevenSegmentDisplay(const gpio_num_t* segmentPins, gpio_num_t enablePin);

    void init();               // configura los GPIO y deja el display apagado
    void show(uint8_t digit);  // escribe los segmentos y habilita ESTE display
    void blank();              // deshabilita este display

    static uint8_t pattern(uint8_t digit);  // bits: g f e d c b a

private:
    void writeSegments(uint8_t bits);

    const gpio_num_t* segmentPins_;
    gpio_num_t        enablePin_;
};

}  // namespace bcd
