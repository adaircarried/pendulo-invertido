// control.h: controlador PD digital (ecuación en diferencias del diseño).
#pragma once

// Salidas del controlador, actualizadas en cada muestra k
extern float thetaReferencia;      // referencia de ángulo  [rad] (0 sin lazo externo)
extern float errorActual;          // e[k] = thetaReferencia - theta[k]   [rad]
extern float errorAnterior;        // e[k-1]                [rad]
extern float aceleracionComando;   // a[k] ya saturada      [m/s²]
extern float velocidadComando;     // v[k] ya saturada      [m/s]

// Prepara el controlador antes de la primera muestra: e[k-1] = e[k] y v = 0.
// Así la primera muestra no ve un salto del error (el péndulo ya estaba inclinado).
void controlReiniciar(float theta);

// Una muestra del controlador: referencia, error, ecuación en diferencias, saturación,
// conversión a velocidad y guardar e[k] como e[k-1].
// posicionCarro [m] solo se usa con el lazo externo (LAZO_EXTERNO = 1).
void controlCalcular(float theta, float posicionCarro);
