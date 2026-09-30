#include "display.hpp"

#include "app_config.hpp"

namespace bcd {

namespace {
// Tabla BCD -> 7 segmentos (bit0 = a ... bit6 = g), catodo comun
constexpr uint8_t kDigitPatterns[10] = {
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F   // 9
};
constexpr uint8_t kDashPattern = 0x40;  // "-" para valores fuera de 0..9

uint32_t levelFor(bool on, int onLevel) {
    return on ? onLevel : !onLevel;
}
}  // namespace

SevenSegmentDisplay::SevenSegmentDisplay(const gpio_num_t* segmentPins, gpio_num_t enablePin)
    : segmentPins_(segmentPins), enablePin_(enablePin) {}

void SevenSegmentDisplay::init() {
    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << enablePin_);
    for (int i = 0; i < config::kSegmentCount; ++i) {
        io.pin_bit_mask |= (1ULL << segmentPins_[i]);
    }
    io.mode         = GPIO_MODE_OUTPUT;
    io.pull_up_en   = GPIO_PULLUP_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&io);

    blank();
    writeSegments(0);
}

uint8_t SevenSegmentDisplay::pattern(uint8_t digit) {
    return (digit <= 9) ? kDigitPatterns[digit] : kDashPattern;
}

void SevenSegmentDisplay::show(uint8_t digit) {
    writeSegments(pattern(digit));
    gpio_set_level(enablePin_, levelFor(true, config::kEnableOnLevel));
}

void SevenSegmentDisplay::blank() {
    gpio_set_level(enablePin_, levelFor(false, config::kEnableOnLevel));
}

void SevenSegmentDisplay::writeSegments(uint8_t bits) {
    for (int i = 0; i < config::kSegmentCount; ++i) {
        const bool on = ((bits >> i) & 0x01) != 0;
        gpio_set_level(segmentPins_[i], levelFor(on, config::kSegmentOnLevel));
    }
}

}  // namespace bcd
