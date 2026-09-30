#pragma once

#include <atomic>
#include <cstdint>

#include "system_state.hpp"

namespace bcd {

// Contador BCD de un digito (0..9) con "vuelta" 9 -> 0 y 0 -> 9.
// No depende de FreeRTOS: se puede probar por separado.
class BcdCounter {
public:
    explicit BcdCounter(uint8_t initial = 0);

    void    step(Direction dir);   // avanza una unidad en la direccion dada
    void    set(uint8_t value);    // fuerza un valor (se limita a 0..9)
    uint8_t value() const;

    // Calculo puro del siguiente valor (util para pruebas)
    static uint8_t next(uint8_t value, Direction dir);

private:
    std::atomic<uint8_t> value_;
};

}  // namespace bcd
