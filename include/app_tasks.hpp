#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buttons.hpp"
#include "counter.hpp"
#include "display.hpp"
#include "system_state.hpp"

namespace bcd {

constexpr int kButtonCount = 4;

// Contexto de la aplicacion. Vive en memoria estatica (main.cpp) y se
// comparte con las tareas unicamente mediante pvParameters.
struct AppContext {
    AppContext();

    SystemState state;  // solo lo escribe el TaskManager
    ButtonFlags flags;  // botones -> TaskManager

    BcdCounter    counter1;
    BcdCounter    counter2;
    CounterConfig counter1Cfg;  // pvParameters de Counter1
    CounterConfig counter2Cfg;  // pvParameters de Counter2

    SevenSegmentDisplay display1;
    SevenSegmentDisplay display2;

    ButtonConfig buttons[kButtonCount];  // pvParameters de cada tarea de boton

    // Handles que conserva el TaskManager para gobernar los contadores
    TaskHandle_t counter1Handle;
    TaskHandle_t counter2Handle;
    TaskHandle_t displayHandle;
    TaskHandle_t managerHandle;
};

// Llamadas desde app_main()
void initContext(AppContext& ctx);
void createApplication(AppContext& ctx);

// Funciones de tarea (firma que exige FreeRTOS)
void counterTask(void* pvParameters);      // pvParameters = CounterConfig*
void buttonTask(void* pvParameters);       // pvParameters = ButtonConfig*
void displayTask(void* pvParameters);      // pvParameters = AppContext*
void taskManagerTask(void* pvParameters);  // pvParameters = AppContext*

}  // namespace bcd
