#include "encoder.h"

#include <Arduino.h>
#include <ESP32Encoder.h>
#include <config.h>

static ESP32Encoder enc;
static int32_t cuentas_previas = 0;
static float theta_dot_filtrada = 0.0f;

static const float RAD_POR_CUENTA = 2.0f * PI / cfg::ENC_CUENTAS_VUELTA;

// Filtro paso bajo de primer orden para la derivada:
//   w[k] = ALFA * w[k-1] + (1 - ALFA) * w_cruda[k],   ALFA = tau / (tau + Ts),   tau = 1 / (2*pi*fc)
static const float TAU = 1.0f / (2.0f * PI * cfg::FC_DERIVADA_HZ);
static const float ALFA = TAU / (TAU + cfg::TS);

void encoderIniciar() {
  ESP32Encoder::useInternalWeakPullResistors =
      cfg::ENC_PULLUP_INTERNA ? puType::up : puType::none;
  enc.attachFullQuad(cfg::PIN_ENC_A, cfg::PIN_ENC_B);
  enc.setFilter(cfg::ENC_FILTRO_PCNT);
  encoderCero();
}

void encoderCero() {
  enc.clearCount();
  cuentas_previas = 0;
  theta_dot_filtrada = 0.0f;
}

LecturaEncoder encoderLeer() {
  LecturaEncoder l;
  l.cuentas = (int32_t)enc.getCount();

  // 1) Ángulo: cuentas medidas desde la vertical arriba (colgando + media vuelta),
  //    llevadas al rango [-media vuelta, +media vuelta) para que theta quede entre -pi y +pi.
  const int32_t vuelta = cfg::ENC_CUENTAS_VUELTA;
  int32_t rel = (l.cuentas - cfg::ENC_CUENTAS_ARRIBA) % vuelta;
  if (rel >= vuelta / 2) rel -= vuelta;
  if (rel < -vuelta / 2) rel += vuelta;
  l.theta = cfg::ENC_SIGNO * rel * RAD_POR_CUENTA;

  // 2) Velocidad: cuentas avanzadas en este periodo / Ts, y luego filtro paso bajo.
  const int32_t delta = l.cuentas - cuentas_previas;
  cuentas_previas = l.cuentas;
  const float theta_dot_cruda = cfg::ENC_SIGNO * delta * RAD_POR_CUENTA / cfg::TS;
  theta_dot_filtrada = ALFA * theta_dot_filtrada + (1.0f - ALFA) * theta_dot_cruda;
  l.theta_dot = theta_dot_filtrada;

  return l;
}
