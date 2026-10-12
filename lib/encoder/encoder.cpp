// encoder.cpp: lectura del ángulo del péndulo.
//
// La librería ESP32Encoder usa el PCNT, un contador de pulsos de hardware del ESP32:
// cuenta los flancos de A y B por sí solo, aunque el procesador esté ocupado.
// Aquí solo leemos ese contador y lo convertimos a ángulo.

#include "encoder.h"

#include <ESP32Encoder.h>

#include "config.h"

static ESP32Encoder contador;  // static: solo se usa dentro de este archivo

void encoderIniciar() {
  if (PULLUP_INTERNA) {
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
  } else {
    ESP32Encoder::useInternalWeakPullResistors = puType::none;
  }
  contador.attachFullQuad(PIN_ENCODER_A, PIN_ENCODER_B);  // cuenta los 4 flancos: 600 x 4 = 2400
  contador.setFilter(FILTRO_ENCODER);
  encoderCero();
}

void encoderCero() {
  contador.clearCount();
}

long encoderCuentas() {
  return (long)contador.getCount();
}

float encoderTheta() {
  // Cuentas medidas desde la vertical arriba (el cero está colgando, media vuelta abajo)
  long cuentasDesdeArriba = encoderCuentas() - CUENTAS_ARRIBA;

  // Llevar a una sola vuelta, entre -1200 y +1200, para que theta quede entre -180° y +180°
  cuentasDesdeArriba = cuentasDesdeArriba % CUENTAS_POR_VUELTA;
  if (cuentasDesdeArriba >= CUENTAS_POR_VUELTA / 2) cuentasDesdeArriba -= CUENTAS_POR_VUELTA;
  if (cuentasDesdeArriba < -CUENTAS_POR_VUELTA / 2) cuentasDesdeArriba += CUENTAS_POR_VUELTA;

  // Cuentas a radianes: una vuelta (2400 cuentas) = 2*pi
  return cuentasDesdeArriba * (2.0 * PI / CUENTAS_POR_VUELTA);
}
