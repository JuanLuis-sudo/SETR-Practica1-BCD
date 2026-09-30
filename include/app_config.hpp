#pragma once

#include <cstdint>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

namespace bcd {
namespace config {

// ------------------------------------------------------------------
// Displays de 7 segmentos (catodo comun, segmentos a..g COMPARTIDOS)
// ------------------------------------------------------------------
constexpr int kSegmentCount = 7;

// Orden: a, b, c, d, e, f, g
constexpr gpio_num_t kSegmentPins[kSegmentCount] = {
    GPIO_NUM_4,  // a
    GPIO_NUM_5,  // b
    GPIO_NUM_6,  // c
    GPIO_NUM_7,  // d
    GPIO_NUM_15,  // e
    GPIO_NUM_16,  // f
    GPIO_NUM_17   // g
};

// Una habilitacion (catodo comun) por display
constexpr gpio_num_t kDisplay1Enable = GPIO_NUM_18;
constexpr gpio_num_t kDisplay2Enable = GPIO_NUM_8;

// Nivel logico que ENCIENDE un segmento (catodo comun -> 1)
constexpr int kSegmentOnLevel = 1;
// Nivel logico que HABILITA un display:
//   1 -> catodo conectado a un transistor NPN (recomendado)
//   0 -> catodo conectado directo al GPIO
constexpr int kEnableOnLevel = 1;

// ------------------------------------------------------------------
// Botones (normalmente abiertos, entre el GPIO y GND, pull-up interno)
// ------------------------------------------------------------------
constexpr gpio_num_t kBtnStartPause = GPIO_NUM_9;
constexpr gpio_num_t kBtnDirection  = GPIO_NUM_10;
constexpr gpio_num_t kBtnSpeed      = GPIO_NUM_11;
constexpr gpio_num_t kBtnMode       = GPIO_NUM_12;

constexpr int kButtonPressedLevel = 0;  // con pull-up: presionado = 0

// ------------------------------------------------------------------
// Tiempos (ms)
// ------------------------------------------------------------------
constexpr uint32_t kPeriodSlowMs     = 500;
constexpr uint32_t kPeriodFastMs     = 250;
constexpr uint32_t kButtonPollMs     = 10;
constexpr uint8_t  kDebounceSamples  = 3;   // 3 x 10 ms = 30 ms estable
constexpr uint32_t kManagerPollMs    = 20;
constexpr uint32_t kDisplayRefreshMs = 2;   // tiempo encendido de cada display

// ------------------------------------------------------------------
// Valores iniciales de los contadores
// ------------------------------------------------------------------
constexpr uint8_t kCounter1Initial = 0;
constexpr uint8_t kCounter2Initial = 9;

// ------------------------------------------------------------------
// FreeRTOS
// ------------------------------------------------------------------
// Todas las tareas de la aplicacion corren en el nucleo 1 (APP_CPU).
constexpr BaseType_t kAppCore = 1;

constexpr uint32_t kStackCounter = 3072;  // bytes (en ESP-IDF el stack va en bytes)
constexpr uint32_t kStackButton  = 3072;
constexpr uint32_t kStackManager = 4096;
constexpr uint32_t kStackDisplay = 2048;

constexpr UBaseType_t kPrioDisplay = 5;  // refresco del multiplexado
constexpr UBaseType_t kPrioManager = 4;  // TaskManager > contadores
constexpr UBaseType_t kPrioButton  = 3;
constexpr UBaseType_t kPrioCounter = 2;

// ------------------------------------------------------------------
// Depuracion
// ------------------------------------------------------------------
constexpr bool kRunSelfTests    = false;  // true: actividades 2 y 3 al arrancar
constexpr bool kLogCounterSteps = false;  // true: imprime cada paso de los contadores

}  // namespace config
}  // namespace bcd
