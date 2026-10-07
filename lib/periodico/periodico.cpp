#include "periodico.h"

#include <Arduino.h>

static void (*funcion)() = nullptr;
static uint32_t periodo = 0;
static TaskHandle_t tarea = nullptr;

static volatile uint32_t periodo_min = 0;
static volatile uint32_t periodo_max = 0;
static volatile bool pedir_reinicio = true;

// Interrupción del timer: solo despierta a la tarea, el trabajo se hace fuera.
static void IRAM_ATTR alDispararTimer() {
  BaseType_t despertar = pdFALSE;
  vTaskNotifyGiveFromISR(tarea, &despertar);
  portYIELD_FROM_ISR(despertar);
}

static void cuerpoTarea(void*) {
  // El timer se configura aquí para que su interrupción quede en este mismo núcleo.
  hw_timer_t* timer = timerBegin(0, 80, true);  // 80 MHz / 80 = 1 tick por microsegundo
  timerAttachInterrupt(timer, &alDispararTimer, true);
  timerAlarmWrite(timer, periodo, true);        // se repite cada `periodo` us
  timerAlarmEnable(timer);

  uint32_t t_anterior = micros();
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // dormir hasta el siguiente disparo

    const uint32_t t = micros();
    const uint32_t dt = t - t_anterior;
    t_anterior = t;
    if (pedir_reinicio) {
      periodo_min = periodo_max = dt;
      pedir_reinicio = false;
    }
    if (dt < periodo_min) periodo_min = dt;
    if (dt > periodo_max) periodo_max = dt;

    funcion();
  }
}

void iniciarTareaPeriodica(void (*fn)(), uint32_t periodo_us, int nucleo, int prioridad) {
  funcion = fn;
  periodo = periodo_us;
  xTaskCreatePinnedToCore(cuerpoTarea, "control", 4096, nullptr, prioridad, &tarea, nucleo);
}

void leerPeriodo(uint32_t& min_us, uint32_t& max_us) {
  min_us = periodo_min;
  max_us = periodo_max;
}

void reiniciarPeriodo() { pedir_reinicio = true; }
