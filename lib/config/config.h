// config.h: pines, parámetros físicos y constantes del controlador.
// Todos los programas (fases) usan este mismo archivo.
#pragma once

#include <Arduino.h>

// ===================================================== Pines del ESP32
const int PIN_ENCODER_A = 32;    // cable verde del encoder
const int PIN_ENCODER_B = 33;    // cable blanco del encoder
const int PIN_STEP = 25;         // pulso de paso al driver
const int PIN_DIR = 26;          // dirección al driver
const int PIN_ENABLE = 27;       // habilita el driver (activo en bajo)
const int PIN_FINAL_IZQ = 18;    // final de carrera izquierdo
const int PIN_FINAL_DER = 19;    // final de carrera derecho

// Finales de carrera cableados normalmente cerrados (NC) entre el pin y GND, con pull-up interna:
// en reposo el pin lee LOW; al presionarse (o si se corta el cable) lee HIGH.
const int NIVEL_FINAL_ACTIVADO = HIGH;

// ===================================================== Encoder (sensor)
const int CUENTAS_POR_VUELTA = 2400;  // 600 PPR x 4 (cuadratura completa)
const int CUENTAS_ARRIBA = 1200;      // vertical arriba = colgando + media vuelta
const int FILTRO_ENCODER = 1023;      // filtro de hardware del PCNT: ignora pulsos < 12.8 us
// true: pull-ups internas del ESP32 (pruebas en la mesa).
// false: pull-ups externas de 4.7k a 3.3 V (montaje final).
const bool PULLUP_INTERNA = true;

// ===================================================== Motor y transmisión
const float PASOS_POR_METRO = 40000.0;  // 200 pasos x 8 micropasos / 0.040 m por vuelta de polea
// Si en la Fase 4 el carro empuja al lado contrario de la caída, cambiar a true.
const bool MOTOR_INVERTIDO = false;

// Autohoming: buscar ambos finales de carrera despacio y dejar el carro al centro
const float VELOCIDAD_HOMING = 0.05;     // [m/s]
const float ACELERACION_HOMING = 0.5;    // [m/s²]
const float RETROCESO_HOMING = 0.01;     // [m] se separa del final después de tocarlo
const unsigned long TIEMPO_MAX_HOMING_MS = 30000;  // si no encuentra el final, aborta

// ===================================================== Controlador (diseño en MATLAB)
const float T = 0.01;  // tiempo de muestreo [s], paso 1 del diseño (100 Hz)

// Ecuación en diferencias, paso 4 del diseño:  u[k] = 635.94 e[k] - 584.27 e[k-1]
// Sale de D(z) = K (z - zc) / z, con K = 635.94 y zc = 0.91875 (paso 3, LGR en z).
const float COEF_ERROR_ACTUAL = 635.94;    // coeficiente de e[k]   = K
const float COEF_ERROR_ANTERIOR = 584.27;  // coeficiente de e[k-1] = K * zc

// ===================================================== Seguridad
const float THETA_MAXIMO = 30.0 * PI / 180.0;   // si |theta| > 30° el péndulo ya cayó: apagar
const float THETA_ARRANQUE = 5.0 * PI / 180.0;  // solo se arranca con |theta| < 5°

// Saturaciones. Valores provisionales hasta medir los máximos en la Fase 2.
#if FASE == 4
const float ACELERACION_MAXIMA = 1.0;  // [m/s²] muy limitada para verificar el signo
const float VELOCIDAD_MAXIMA = 0.1;    // [m/s]
#else
const float ACELERACION_MAXIMA = 10.0;  // [m/s²] provisional (diseño pide ~9.1 ante 10°)
const float VELOCIDAD_MAXIMA = 0.5;     // [m/s]  provisional (simulación: ~0.46 ante 10°)
#endif
