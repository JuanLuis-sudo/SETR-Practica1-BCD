#include "counter.hpp"

namespace bcd {

namespace {
constexpr uint8_t kMinDigit = 0;
constexpr uint8_t kMaxDigit = 9;

uint8_t clampDigit(uint8_t value) {
    return (value > kMaxDigit) ? kMaxDigit : value;
}
}  // namespace

BcdCounter::BcdCounter(uint8_t initial) : value_(clampDigit(initial)) {}

uint8_t BcdCounter::next(uint8_t value, Direction dir) {
    if (dir == Direction::Up) {
        return (value >= kMaxDigit) ? kMinDigit : static_cast<uint8_t>(value + 1);
    }
    return (value <= kMinDigit) ? kMaxDigit : static_cast<uint8_t>(value - 1);
}

void BcdCounter::step(Direction dir) {
    uint8_t current = value_.load();
    const uint8_t following = next(current, dir);
    // Si el TaskManager cambio el valor con set() mientras se calculaba,
    // el intercambio falla y este paso se descarta: prevalece el ajuste.
    value_.compare_exchange_strong(current, following);
}

void BcdCounter::set(uint8_t value) {
    value_.store(clampDigit(value));
}

uint8_t BcdCounter::value() const {
    return value_.load();
}

}  // namespace bcd
