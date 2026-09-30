# Contadores BCD concurrentes con FreeRTOS (ESP32)

Dos contadores BCD (0-9) independientes mostrados en dos displays de 7 segmentos
(cátodo común, segmentos compartidos y multiplexados). Gobernados por un
TaskManager mediante `TaskHandle_t`, `vTaskSuspend()` y `vTaskResume()`.
Sin queues, semáforos, mutex, Event Groups ni Software Timers.

## Conexiones (editar en `include/app_config.hpp`)

| Señal | GPIO |
|---|---|
| Segmentos a, b, c, d, e, f, g | 13, 14, 27, 26, 25, 33, 32 (con resistencia de 220-330 Ω cada uno) |
| Habilitación Display 1 / Display 2 | 23 / 22 (a la base de un NPN con 1 kΩ; el colector va al cátodo común) |
| Start/Pause, Dirección, Velocidad, Modo | 4, 18, 19, 21 (botón entre el GPIO y GND; pull-up interno) |

## Tareas

| Tarea | Función | pvParameters | Prioridad |
|---|---|---|---|
| Counter1 | `counterTask` | `CounterConfig*` (counter1Cfg) | 2 |
| Counter2 | `counterTask` | `CounterConfig*` (counter2Cfg) | 2 |
| BtnStartPause, BtnDireccion, BtnVelocidad, BtnModo | `buttonTask` | `ButtonConfig*` | 3 |
| TaskManager | `taskManagerTask` | `AppContext*` | 4 |
| Display | `displayTask` | `AppContext*` | 5 |

Todas las tareas corren en el núcleo 1. La tarea `Display` hace el multiplexado; es
necesaria porque los contadores se suspenden y, aun así, los displays deben seguir
mostrando su valor.

## Flujo de datos

1. Cada tarea de botón hace antirrebote, detecta el flanco de presión y solo levanta su flag (`ButtonFlags`).
2. El TaskManager consume los flags (`exchange(false)`), actualiza `SystemState` y escribe en `CounterConfig` (dirección y periodo).
3. Counter1 y Counter2 leen su `CounterConfig` y avanzan su `BcdCounter`.
4. En pausa, los flags de Dirección, Velocidad y Modo se consumen y se ignoran.

## Uso de handles

- `createApplication()` guarda `counter1Handle` y `counter2Handle` al crear las tareas.
- Al arrancar, el TaskManager llama `vTaskSuspend()` sobre ambos: el sistema inicia en PAUSA.
- Start/Pause: `vTaskSuspend(counter1Handle)` y `vTaskSuspend(counter2Handle)` para pausar;
  `vTaskResume()` sobre ambos para reanudar. El valor se conserva porque vive en `BcdCounter`.
- Velocidad y Modo: el TaskManager suspende ambos contadores, modifica su configuración (y
  alinea los valores en OPPOSITE -> SAME) y los reanuda juntos, para que queden en fase.
- Por UART se imprime `eTaskGetState()`: `Suspended` tras pausar y `Ready` justo después de reanudar.

Detalle: `vTaskResume()` saca a la tarea de su `vTaskDelay()` antes de tiempo. Por eso el
TaskManager levanta `restartPeriod` antes de reanudar, y la tarea Counter, al ver el flag,
reinicia el periodo completo en lugar de contar de inmediato.

## Tabla de comportamiento

| Modo | Dirección base | Display 1 | Display 2 |
|---|---|---|---|
| OPPOSITE (0) | UP | UP | DOWN |
| OPPOSITE (0) | DOWN | DOWN | UP |
| SAME (1) | UP | UP | UP |
| SAME (1) | DOWN | DOWN | DOWN |

Al pasar de OPPOSITE a SAME: con DOWN ambos displays toman el dígito menor; con UP, el mayor.

## Pruebas

En `app_config.hpp`, `kRunSelfTests = true` prueba `BcdCounter` (por UART) y muestra 0-9 en cada
display por separado antes de crear las tareas. `kLogCounterSteps = true` imprime cada paso.

## Tabla de comportamiento

## Integrantes

- Oswaldo Martin Alvarado Esparza - 11036
- Luis Roberto Casas Caballero - 10072
- Juan Luis Trejo Garcia - 11163
