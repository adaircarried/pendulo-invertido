// Tarea periódica de tiempo real: un timer de hardware la despierta cada `periodo_us`.
#pragma once

#include <stdint.h>

// Ejecuta fn() cada periodo_us en el núcleo y con la prioridad indicados.
void iniciarTareaPeriodica(void (*fn)(), uint32_t periodo_us, int nucleo, int prioridad);

// Periodo real mínimo y máximo medido [us] desde el último reinicio (debe estar cerca de Ts).
void leerPeriodo(uint32_t& min_us, uint32_t& max_us);
void reiniciarPeriodo();
