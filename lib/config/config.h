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

// false: sin finales de carrera. El carro se pone a mano en el centro y el comando 'h'
// toma esa posición como 0. Los límites del riel se vigilan por software (cuenta de pasos).
const bool USAR_FINALES_DE_CARRERA = false;
const float RECORRIDO_UTIL = 0.88;  // [m] recorrido útil del carro, medido a mano
const float MARGEN_LIMITE = 0.0;    // [m] sin margen: los 0.88 m medidos ya dejan holgura física en cada extremo

// ===================================================== Encoder (sensor)
const int CUENTAS_POR_VUELTA = 2400;  // 600 PPR x 4 (cuadratura completa)
const int CUENTAS_ARRIBA = 1200;      // vertical arriba = colgando + media vuelta
const int FILTRO_ENCODER = 1023;      // filtro de hardware del PCNT: ignora pulsos < 12.8 us
// true: pull-ups internas del ESP32 (pruebas en la mesa).
// false: pull-ups externas a 3.3 V, de 1k a 10k (montaje final).
const bool PULLUP_INTERNA = true;

// ===================================================== Motor y transmisión
const float PASOS_POR_METRO = 40000.0;  // 200 pasos x 8 micropasos / 0.040 m por vuelta de polea
// true: medido en la Fase 2 para que el sentido positivo del motor vaya hacia DER (+).
// Si en la Fase 4 el carro empuja al lado contrario de la caída, cambiarlo.
const bool MOTOR_INVERTIDO = true;

// Autohoming (solo con finales de carrera): buscarlos despacio y dejar el carro al centro
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

// ===================================================== Lazo externo de posición (cascada)
// LAZO_EXTERNO lo define el entorno de platformio.ini:
//   0 = solo el diseño de la clase (referencia theta = 0)
//   1 = cascada: el lazo externo genera la referencia de ángulo para regresar el carro al centro
#ifndef LAZO_EXTERNO
#define LAZO_EXTERNO 0
#endif
const float GANANCIA_POSICION = 0.05;   // [rad/m]       w² = g·0.05  -> w ≈ 0.7 rad/s (≈17 veces más lento que el lazo interno)
const float GANANCIA_VELOCIDAD = 0.10;  // [rad/(m/s)]   2·zeta·w = g·0.10 -> zeta ≈ 0.7
const float THETA_REF_MAXIMA = 3.0 * PI / 180.0;  // el lazo externo pide como máximo 3° de inclinación
// Filtro de primer orden de la referencia: sin él, la referencia cambiaba en cada muestra y la parte
// derivativa del lazo interno (el término con e[k-1]) la amplificaba ~2.7 veces; el carro se saturaba.
const float TAU_REFERENCIA = 0.5;  // [s] constante de tiempo del filtro (probada en simulación con retardo del motor)

// ===================================================== Swing-up (arranque desde colgando)
// SWING_UP lo define el entorno de platformio.ini: 1 solo en fase6_swingup.
// Control por energía (no es parte del diseño de la clase): mece el péndulo hasta subirlo y,
// cerca de la vertical, cambia al control de equilibrio (diseño de la clase + lazo externo).
#ifndef SWING_UP
#define SWING_UP 0
#endif
const float GRAVEDAD = 9.81;            // [m/s²]
const float LONGITUD_PENDULO = 0.342;   // [m] pivote a centro de masa (datos del sistema)
const float GANANCIA_ENERGIA = 15.0;    // [1/m] a = 15·E: colgando (E = -2·g·l = -6.7) ya pide el empuje máximo
const float CENTRADO_POSICION = 2.0;    // [1/s²] resorte virtual: mientras sube, el carro no se aleja del centro
const float CENTRADO_VELOCIDAD = 6.0;   // [1/s]  amortiguador virtual
// El bombeo se limita a 4.5 m/s² para que el centrado no quede tapado por la saturación de 5.5 m/s²
// (sin este límite el carro se iba hacia la IZQ mientras subía).
const float LIMITE_BOMBEO = 4.5;        // [m/s²]
const float ANGULO_ATRAPE = 15.0 * PI / 180.0;  // a menos de 15° de la vertical pasa al control de equilibrio
// Valores elegidos con simulación (cuantización, retardo y fricción del eje): éxito ~50 %, depende de la fricción real.
// Usa las mismas saturaciones (5.5 m/s², 0.4 m/s) que el control de equilibrio.

// Saturaciones (Fase 2: sin perder pasos hasta 7 m/s² a 0.5 m/s; con 8 m/s² ya pierde pasos).
#if FASE == 4
const float ACELERACION_MAXIMA = 1.0;  // [m/s²] muy limitada para verificar el signo
const float VELOCIDAD_MAXIMA = 0.1;    // [m/s]
#else
const float ACELERACION_MAXIMA = 5.5;  // [m/s²] ~80 % del máximo medido (7 m/s²)
const float VELOCIDAD_MAXIMA = 0.4;    // [m/s]  debajo de los 0.5 m/s probados
#endif
