// Encoder del péndulo (LPD3806-600BM) leído por el contador de pulsos (PCNT) del ESP32.
#pragma once

#include <stdint.h>

struct LecturaEncoder {
  int32_t cuentas;  // desde la referencia (péndulo colgando)
  float theta;      // [rad], 0 = vertical arriba, entre -pi y +pi
  float theta_dot;  // [rad/s], derivada filtrada
};

// Configura el PCNT en cuadratura completa (x4) con filtro de hardware.
void encoderIniciar();

// Pone la cuenta en 0. Llamar con el péndulo colgando y quieto.
void encoderCero();

// Lee la cuenta y calcula theta y theta_dot. Llamar una vez por periodo Ts,
// siempre desde la tarea de control (guarda la cuenta anterior para la derivada).
LecturaEncoder encoderLeer();
