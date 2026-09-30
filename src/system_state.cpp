#include "system_state.hpp"

#include "app_config.hpp"

namespace bcd {

void initSystemState(SystemState& state) {
    state.run           = RunState::Paused;   // arranca en pausa
    state.baseDirection = Direction::Up;
    state.speed         = Speed::Slow;
    state.mode          = CouplingMode::Opposite;
}

void initButtonFlags(ButtonFlags& flags) {
    flags.startPause.store(false);
    flags.direction.store(false);
    flags.speed.store(false);
    flags.mode.store(false);
}

void initCounterConfig(CounterConfig& cfg, const char* name, BcdCounter* counter) {
    cfg.name    = name;
    cfg.counter = counter;
    cfg.direction.store(Direction::Up);
    cfg.periodMs.store(config::kPeriodSlowMs);
    cfg.restartPeriod.store(false);
}

Direction opposite(Direction dir) {
    return (dir == Direction::Up) ? Direction::Down : Direction::Up;
}

// Display 1 siempre sigue la direccion base
Direction counter1Direction(const SystemState& state) {
    return state.baseDirection;
}

// Display 2: SAME -> misma direccion, OPPOSITE -> direccion contraria
Direction counter2Direction(const SystemState& state) {
    return (state.mode == CouplingMode::Same) ? state.baseDirection
                                              : opposite(state.baseDirection);
}

uint32_t periodMsFor(Speed speed) {
    return (speed == Speed::Slow) ? config::kPeriodSlowMs : config::kPeriodFastMs;
}

const char* toString(RunState state) {
    return (state == RunState::Running) ? "RUN" : "PAUSA";
}

const char* toString(Direction dir) {
    return (dir == Direction::Up) ? "UP" : "DOWN";
}

const char* toString(Speed speed) {
    return (speed == Speed::Slow) ? "500 ms" : "250 ms";
}

const char* toString(CouplingMode mode) {
    return (mode == CouplingMode::Same) ? "SAME(1)" : "OPPOSITE(0)";
}

}  // namespace bcd
