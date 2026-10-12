// control.cpp: controlador PD digital D(z) = K (z - zc) / z, implementado como
// su ecuación en diferencias (paso 4 del diseño).

#include "control.h"

#include "config.h"

float errorActual = 0.0;
float errorAnterior = 0.0;
float aceleracionComando = 0.0;
float velocidadComando = 0.0;

void controlReiniciar(float theta) {
  errorAnterior = 0.0 - theta;
  aceleracionComando = 0.0;
  velocidadComando = 0.0;
}

void controlCalcular(float theta) {
  // Error respecto a la referencia theta = 0 (regulación)
  errorActual = 0.0 - theta;

  // Ecuación en diferencias del controlador (paso 4): u[k] = 635.94 e[k] - 584.27 e[k-1]
  float u = COEF_ERROR_ACTUAL * errorActual - COEF_ERROR_ANTERIOR * errorAnterior;

  // El signo de la planta se absorbió en el diseño: aceleración física del carro a = -u
  aceleracionComando = -u;

  // Saturación: el modelo solo es válido si el stepper no pierde pasos
  aceleracionComando = constrain(aceleracionComando, -ACELERACION_MAXIMA, ACELERACION_MAXIMA);

  // Aceleración a velocidad (el motor se maneja por velocidad): v[k] = v[k-1] + a[k]·T
  velocidadComando = velocidadComando + aceleracionComando * T;
  velocidadComando = constrain(velocidadComando, -VELOCIDAD_MAXIMA, VELOCIDAD_MAXIMA);

  // e[k] pasa a ser e[k-1] para la siguiente muestra
  errorAnterior = errorActual;
}
