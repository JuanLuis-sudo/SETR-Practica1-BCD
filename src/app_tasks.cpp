#include "app_tasks.hpp"

#include <cinttypes>

#include "app_config.hpp"
#include "esp_log.h"

namespace bcd {

namespace {

const char* const TAG = "APP";

// Nunca devuelve 0 ticks: vTaskDelay(0) no bloquea la tarea.
TickType_t msToTicks(uint32_t ms) {
    const TickType_t ticks = pdMS_TO_TICKS(ms);
    return (ticks == 0) ? 1 : ticks;
}

const char* taskStateName(eTaskState state) {
    switch (state) {
        case eRunning:   return "Running";
        case eReady:     return "Ready";
        case eBlocked:   return "Blocked";
        case eSuspended: return "Suspended";
        case eDeleted:   return "Deleted";
        default:         return "Invalid";
    }
}

// ------------------------------------------------------------------
// Registro por UART
// ------------------------------------------------------------------
void logSystemState(const AppContext& ctx) {
    ESP_LOGI(TAG, "   Estado=%s | DirBase=%s | Periodo=%s | Modo=%s | D1=%u(%s) D2=%u(%s)",
             toString(ctx.state.run),
             toString(ctx.state.baseDirection),
             toString(ctx.state.speed),
             toString(ctx.state.mode),
             static_cast<unsigned>(ctx.counter1.value()),
             toString(ctx.counter1Cfg.direction.load()),
             static_cast<unsigned>(ctx.counter2.value()),
             toString(ctx.counter2Cfg.direction.load()));
}

void logCounterTaskStates(const AppContext& ctx) {
    ESP_LOGI(TAG, "   eTaskGetState -> Counter1: %s | Counter2: %s",
             taskStateName(eTaskGetState(ctx.counter1Handle)),
             taskStateName(eTaskGetState(ctx.counter2Handle)));
}

// ------------------------------------------------------------------
// Acciones del TaskManager
// ------------------------------------------------------------------

// Detiene Counter1 y Counter2 usando sus handles.
void suspendCounters(AppContext& ctx) {
    vTaskSuspend(ctx.counter1Handle);
    vTaskSuspend(ctx.counter2Handle);
}

// Reanuda ambos contadores. Antes les pide reiniciar su periodo para que
// arranquen juntos (en fase) y esperen un periodo completo antes de contar.
void resumeCounters(AppContext& ctx) {
    ctx.counter1Cfg.restartPeriod.store(true);
    ctx.counter2Cfg.restartPeriod.store(true);
    vTaskResume(ctx.counter1Handle);
    vTaskResume(ctx.counter2Handle);
}

void applyDirections(AppContext& ctx) {
    ctx.counter1Cfg.direction.store(counter1Direction(ctx.state));
    ctx.counter2Cfg.direction.store(counter2Direction(ctx.state));
}

void applySpeed(AppContext& ctx) {
    const uint32_t periodMs = periodMsFor(ctx.state.speed);
    ctx.counter1Cfg.periodMs.store(periodMs);
    ctx.counter2Cfg.periodMs.store(periodMs);
}

// OPPOSITE -> SAME: con cuenta DOWN ambos van al digito menor,
// con cuenta UP ambos van al digito mayor.
void alignCounters(AppContext& ctx) {
    const uint8_t v1 = ctx.counter1.value();
    const uint8_t v2 = ctx.counter2.value();
    uint8_t target;
    if (ctx.state.baseDirection == Direction::Down) {
        target = (v1 < v2) ? v1 : v2;
    } else {
        target = (v1 > v2) ? v1 : v2;
    }
    ctx.counter1.set(target);
    ctx.counter2.set(target);
}

void handleStartPause(AppContext& ctx) {
    if (ctx.state.run == RunState::Running) {
        suspendCounters(ctx);
        ctx.state.run = RunState::Paused;
        ESP_LOGI(TAG, "[START/PAUSE] -> PAUSA: vTaskSuspend(Counter1), vTaskSuspend(Counter2)");
    } else {
        ctx.state.run = RunState::Running;
        resumeCounters(ctx);
        ESP_LOGI(TAG, "[START/PAUSE] -> RUN: vTaskResume(Counter1), vTaskResume(Counter2)");
    }
    logSystemState(ctx);
    logCounterTaskStates(ctx);
}

void handleDirection(AppContext& ctx) {
    ctx.state.baseDirection = opposite(ctx.state.baseDirection);
    applyDirections(ctx);
    ESP_LOGI(TAG, "[DIRECCION] Direccion base -> %s", toString(ctx.state.baseDirection));
    logSystemState(ctx);
}

void handleSpeed(AppContext& ctx) {
    suspendCounters(ctx);
    ctx.state.speed = (ctx.state.speed == Speed::Slow) ? Speed::Fast : Speed::Slow;
    applySpeed(ctx);
    resumeCounters(ctx);  // el nuevo periodo aplica de inmediato
    ESP_LOGI(TAG, "[VELOCIDAD] Periodo -> %s", toString(ctx.state.speed));
    logSystemState(ctx);
}

void handleMode(AppContext& ctx) {
    // Nadie cuenta mientras se ajustan valores y direcciones
    suspendCounters(ctx);

    const CouplingMode previous = ctx.state.mode;
    const uint8_t v1 = ctx.counter1.value();
    const uint8_t v2 = ctx.counter2.value();

    ctx.state.mode = (previous == CouplingMode::Opposite) ? CouplingMode::Same
                                                          : CouplingMode::Opposite;
    const bool mustAlign = (previous == CouplingMode::Opposite) &&
                           (ctx.state.mode == CouplingMode::Same);
    if (mustAlign) {
        alignCounters(ctx);
    }
    applyDirections(ctx);

    resumeCounters(ctx);

    ESP_LOGI(TAG, "[MODO] %s -> %s", toString(previous), toString(ctx.state.mode));
    if (mustAlign) {
        ESP_LOGI(TAG, "   Ajuste (%s): D1=%u, D2=%u -> ambos en %u",
                 toString(ctx.state.baseDirection),
                 static_cast<unsigned>(v1),
                 static_cast<unsigned>(v2),
                 static_cast<unsigned>(ctx.counter1.value()));
    }
    logSystemState(ctx);
}

// ------------------------------------------------------------------
// Pruebas de las actividades 2 y 3 (config::kRunSelfTests)
// ------------------------------------------------------------------
bool expectSteps(BcdCounter& counter, Direction dir, const uint8_t* expected, int count) {
    bool ok = true;
    for (int i = 0; i < count; ++i) {
        counter.step(dir);
        ESP_LOGI(TAG, "   step(%s) -> %u (esperado %u)", toString(dir),
                 static_cast<unsigned>(counter.value()),
                 static_cast<unsigned>(expected[i]));
        ok = ok && (counter.value() == expected[i]);
    }
    return ok;
}

void runSelfTests(AppContext& ctx) {
    // Actividad 3: BcdCounter no usa FreeRTOS
    ESP_LOGI(TAG, "=== Prueba BcdCounter ===");
    BcdCounter counter(8);
    const uint8_t expectedUp[]   = {9, 0, 1};
    const uint8_t expectedDown[] = {0, 9, 8};
    bool ok = expectSteps(counter, Direction::Up, expectedUp, 3);
    ok = expectSteps(counter, Direction::Down, expectedDown, 3) && ok;
    counter.set(15);
    ok = ok && (counter.value() == 9);
    ESP_LOGI(TAG, "BcdCounter: %s", ok ? "OK" : "FALLA");

    // Actividad 2: cada display por separado mostrando 0..9
    ESP_LOGI(TAG, "=== Prueba de displays ===");
    SevenSegmentDisplay* displays[2] = {&ctx.display1, &ctx.display2};
    for (int d = 0; d < 2; ++d) {
        ESP_LOGI(TAG, "Display %d: 0..9", d + 1);
        for (uint8_t digit = 0; digit <= 9; ++digit) {
            displays[d]->show(digit);
            vTaskDelay(msToTicks(400));
        }
        displays[d]->blank();
    }
}

void createPinnedTask(TaskFunction_t function, const char* name, uint32_t stackBytes,
                      void* parameters, UBaseType_t priority, TaskHandle_t* handle) {
    const BaseType_t result = xTaskCreatePinnedToCore(function, name, stackBytes, parameters,
                                                      priority, handle, config::kAppCore);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "No se pudo crear la tarea %s", name);
    }
    configASSERT(result == pdPASS);
}

}  // namespace

// ------------------------------------------------------------------
// Contexto
// ------------------------------------------------------------------
AppContext::AppContext()
    : counter1(config::kCounter1Initial),
      counter2(config::kCounter2Initial),
      display1(config::kSegmentPins, config::kDisplay1Enable),
      display2(config::kSegmentPins, config::kDisplay2Enable),
      counter1Handle(nullptr),
      counter2Handle(nullptr),
      displayHandle(nullptr),
      managerHandle(nullptr) {}

void initContext(AppContext& ctx) {
    initSystemState(ctx.state);
    initButtonFlags(ctx.flags);

    initCounterConfig(ctx.counter1Cfg, "Counter1", &ctx.counter1);
    initCounterConfig(ctx.counter2Cfg, "Counter2", &ctx.counter2);
    applyDirections(ctx);  // direcciones y periodo segun el estado inicial
    applySpeed(ctx);

    ctx.buttons[0] = {"StartPause", config::kBtnStartPause, &ctx.flags.startPause};
    ctx.buttons[1] = {"Direccion",  config::kBtnDirection,  &ctx.flags.direction};
    ctx.buttons[2] = {"Velocidad",  config::kBtnSpeed,      &ctx.flags.speed};
    ctx.buttons[3] = {"Modo",       config::kBtnMode,       &ctx.flags.mode};

    ctx.display1.init();
    ctx.display2.init();

    ESP_LOGI(TAG, "Contexto inicializado");
}

void createApplication(AppContext& ctx) {
    if (config::kRunSelfTests) {
        runSelfTests(ctx);
    }

    // Counter1 y Counter2: MISMA funcion de tarea, distinto pvParameters.
    // Se guardan sus handles para que el TaskManager los gobierne.
    createPinnedTask(counterTask, "Counter1", config::kStackCounter,
                     &ctx.counter1Cfg, config::kPrioCounter, &ctx.counter1Handle);
    createPinnedTask(counterTask, "Counter2", config::kStackCounter,
                     &ctx.counter2Cfg, config::kPrioCounter, &ctx.counter2Handle);

    // Cuatro tareas de boton: misma funcion, distinto ButtonConfig
    const char* const buttonTaskNames[kButtonCount] = {"BtnStartPause", "BtnDireccion",
                                                       "BtnVelocidad", "BtnModo"};
    for (int i = 0; i < kButtonCount; ++i) {
        createPinnedTask(buttonTask, buttonTaskNames[i], config::kStackButton,
                         &ctx.buttons[i], config::kPrioButton, nullptr);
    }

    createPinnedTask(displayTask, "Display", config::kStackDisplay,
                     &ctx, config::kPrioDisplay, &ctx.displayHandle);

    // Se crea al final: cuando arranca, los handles de los contadores ya existen
    createPinnedTask(taskManagerTask, "TaskManager", config::kStackManager,
                     &ctx, config::kPrioManager, &ctx.managerHandle);
}

// ------------------------------------------------------------------
// Tareas
// ------------------------------------------------------------------
void counterTask(void* pvParameters) {
    CounterConfig* cfg = static_cast<CounterConfig*>(pvParameters);
    ESP_LOGI(TAG, "%s creada en el nucleo %d", cfg->name, static_cast<int>(xPortGetCoreID()));

    for (;;) {
        cfg->restartPeriod.store(false);
        vTaskDelay(msToTicks(cfg->periodMs.load()));

        // Si el TaskManager suspendio y reanudo esta tarea mientras esperaba,
        // vTaskResume() corta el retardo antes de tiempo. En ese caso NO se
        // cuenta: se reinicia el periodo completo.
        if (cfg->restartPeriod.load()) {
            continue;
        }

        cfg->counter->step(cfg->direction.load());

        if (config::kLogCounterSteps) {
            ESP_LOGI(TAG, "%s -> %u", cfg->name, static_cast<unsigned>(cfg->counter->value()));
        }
    }
}

void buttonTask(void* pvParameters) {
    ButtonConfig* cfg = static_cast<ButtonConfig*>(pvParameters);
    Button button(cfg->pin, config::kDebounceSamples);
    button.init();

    for (;;) {
        if (button.update()) {       // flanco de presion ya sin rebote
            cfg->flag->store(true);  // solo publica; el TaskManager interpreta
            ESP_LOGI(TAG, "Boton %s presionado", cfg->name);
        }
        vTaskDelay(msToTicks(config::kButtonPollMs));
    }
}

void displayTask(void* pvParameters) {
    AppContext* ctx = static_cast<AppContext*>(pvParameters);
    const TickType_t onTime = msToTicks(config::kDisplayRefreshMs);

    // Multiplexado: sigue refrescando aunque los contadores esten suspendidos
    for (;;) {
        ctx->display2.blank();
        ctx->display1.show(ctx->counter1.value());
        vTaskDelay(onTime);

        ctx->display1.blank();
        ctx->display2.show(ctx->counter2.value());
        vTaskDelay(onTime);
    }
}

void taskManagerTask(void* pvParameters) {
    AppContext* ctx = static_cast<AppContext*>(pvParameters);
    ButtonFlags& flags = ctx->flags;

    // Estado inicial: PAUSA. El TaskManager toma el control de los contadores.
    suspendCounters(*ctx);
    ESP_LOGI(TAG, "TaskManager listo. Sistema en PAUSA: presione Start/Pause");
    logSystemState(*ctx);
    logCounterTaskStates(*ctx);

    for (;;) {
        // Consumir = leer y limpiar el flag en una sola operacion
        if (flags.startPause.exchange(false)) {
            handleStartPause(*ctx);
        }

        const bool direction = flags.direction.exchange(false);
        const bool speed     = flags.speed.exchange(false);
        const bool mode      = flags.mode.exchange(false);

        if (ctx->state.run == RunState::Paused) {
            if (direction || speed || mode) {
                ESP_LOGW(TAG, "Sistema en PAUSA: Direccion/Velocidad/Modo se ignoran");
            }
        } else {
            if (direction) {
                handleDirection(*ctx);
            }
            if (speed) {
                handleSpeed(*ctx);
            }
            if (mode) {
                handleMode(*ctx);
            }
        }

        vTaskDelay(msToTicks(config::kManagerPollMs));
    }
}

}  // namespace bcd
