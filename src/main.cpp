#include "app_tasks.hpp"

namespace {
// Contexto con duracion estatica: debe seguir existiendo despues de que
// app_main() termine, porque las tareas lo usan por pvParameters.
bcd::AppContext g_context;
}  // namespace

extern "C" void app_main(void) {
    bcd::initContext(g_context);
    bcd::createApplication(g_context);
}
