// control.cpp: controlador PD digital D(z) = K (z - zc) / z, implementado como
// su ecuación en diferencias (paso 4 del diseño).

#include "control.h"

#include "config.h"

float thetaReferencia = 0.0;
float errorActual = 0.0;
float errorAnterior = 0.0;
float aceleracionComando = 0.0;
float velocidadComando = 0.0;
float energiaPendulo = 0.0;
static float thetaAnterior = 0.0;  // para estimar la velocidad angular en el swing-up

void controlReiniciar(float theta) {
  thetaReferencia = 0.0;
  errorAnterior = 0.0 - theta;
  aceleracionComando = 0.0;
  velocidadComando = 0.0;
}

void controlCalcular(float theta, float posicionCarro) {
  if (LAZO_EXTERNO) {
    // Lazo externo (cascada): si el carro se aleja del centro, pide una pequeña
    // inclinación hacia el centro; para sostenerla, el lazo interno regresa el carro.
    float thetaDeseada = -(GANANCIA_POSICION * posicionCarro + GANANCIA_VELOCIDAD * velocidadComando);
    thetaDeseada = constrain(thetaDeseada, -THETA_REF_MAXIMA, THETA_REF_MAXIMA);

    // Filtro de primer orden: la referencia se acerca suave a thetaDeseada (constante TAU_REFERENCIA)
    thetaReferencia = thetaReferencia + (T / TAU_REFERENCIA) * (thetaDeseada - thetaReferencia);
  } else {
    // Diseño de la clase: regulación, theta = 0
    thetaReferencia = 0.0;
  }

  // Error respecto a la referencia
  errorActual = thetaReferencia - theta;

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

void controlAtrapar(float theta) {
  thetaReferencia = 0.0;
  errorAnterior = 0.0 - theta;
}

void swingupReiniciar(float theta) {
  thetaAnterior = theta;
  aceleracionComando = 0.0;
  velocidadComando = 0.0;
}

void swingupCalcular(float theta, float posicionCarro) {
  // Velocidad angular por diferencia de dos muestras. Al pasar por abajo theta brinca de +180° a -180°:
  // se corrige sumando o restando una vuelta.
  float cambio = theta - thetaAnterior;
  if (cambio > PI) cambio -= 2.0 * PI;
  if (cambio < -PI) cambio += 2.0 * PI;
  float thetaPunto = cambio / T;
  thetaAnterior = theta;

  // Energía del péndulo por unidad de masa: 0 en la vertical arriba, -2·g·l colgando quieto
  energiaPendulo = 0.5 * LONGITUD_PENDULO * LONGITUD_PENDULO * thetaPunto * thetaPunto
                 + GRAVEDAD * LONGITUD_PENDULO * (cos(theta) - 1.0);

  // Bombeo de energía: empujar en el sentido que hace crecer la energía (como impulsar un columpio).
  // dE/dt = -l·θ'·a·cos θ, así que con E < 0 conviene a = k·E·signo(θ'·cos θ).
  float signo = 1.0;
  if (thetaPunto * cos(theta) < 0) signo = -1.0;
  aceleracionComando = GANANCIA_ENERGIA * energiaPendulo * signo;

  // Centrado: resorte y amortiguador virtuales para que el carro no se salga del riel
  aceleracionComando = aceleracionComando - CENTRADO_POSICION * posicionCarro - CENTRADO_VELOCIDAD * velocidadComando;

  // Mismas saturaciones y misma conversión a velocidad que el control de equilibrio
  aceleracionComando = constrain(aceleracionComando, -ACELERACION_MAXIMA, ACELERACION_MAXIMA);
  velocidadComando = velocidadComando + aceleracionComando * T;
  velocidadComando = constrain(velocidadComando, -VELOCIDAD_MAXIMA, VELOCIDAD_MAXIMA);
}
