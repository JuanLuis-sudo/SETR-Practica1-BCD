#pragma once

#include <atomic>
#include <cstdint>

namespace bcd {

class BcdCounter;  // definida en counter.hpp

// ------------------------------------------------------------------
// Estados y modos (enum class)
// ------------------------------------------------------------------
enum class RunState { Paused, Running };
enum class Direction { Up, Down };
enum class Speed { Slow, Fast };                      // 500 ms / 250 ms
enum class CouplingMode { Opposite = 0, Same = 1 };   // MODE=0 / MODE=1

// ------------------------------------------------------------------
// Estado global del sistema. SOLO lo escribe el TaskManager.
// ------------------------------------------------------------------
struct SystemState {
    RunState     run;
    Direction    baseDirection;  // direccion del Display 1
    Speed        speed;
    CouplingMode mode;
};

// ------------------------------------------------------------------
// Configuracion de un contador: se entrega a la tarea Counter mediante
// pvParameters. La escribe el TaskManager y la lee la tarea Counter.
// Se usan std::atomic (variables sin bloqueo), NO mutex ni semaforos.
// ------------------------------------------------------------------
struct CounterConfig {
    const char*            name;
    BcdCounter*            counter;
    std::atomic<Direction> direction;
    std::atomic<uint32_t>  periodMs;
    std::atomic<bool>      restartPeriod;  // el TaskManager pide reiniciar el periodo
};

// ------------------------------------------------------------------
// Flags simples: los publican las tareas de botones y los consume
// (lee y limpia) unicamente el TaskManager.
// ------------------------------------------------------------------
struct ButtonFlags {
    std::atomic<bool> startPause;
    std::atomic<bool> direction;
    std::atomic<bool> speed;
    std::atomic<bool> mode;
};

// Inicializacion
void initSystemState(SystemState& state);
void initButtonFlags(ButtonFlags& flags);
void initCounterConfig(CounterConfig& cfg, const char* name, BcdCounter* counter);

// Reglas del sistema
Direction opposite(Direction dir);
Direction counter1Direction(const SystemState& state);
Direction counter2Direction(const SystemState& state);
uint32_t  periodMsFor(Speed speed);

// Texto para UART
const char* toString(RunState state);
const char* toString(Direction dir);
const char* toString(Speed speed);
const char* toString(CouplingMode mode);

}  // namespace bcd
