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

// Al pasar del swing-up al equilibrio: e[k-1] = e[k] para no ver un salto, sin frenar el carro.
void controlAtrapar(float theta);

// Swing-up (solo fase6_swingup): mece el carro para subir el péndulo desde colgando.
extern float energiaPendulo;       // E = ½·l²·θ'² + g·l·(cos θ − 1): 0 arriba, −2·g·l colgando [m²/s²]
void swingupReiniciar(float theta);
void swingupCalcular(float theta, float posicionCarro);

// Una muestra del controlador: referencia, error, ecuación en diferencias, saturación,
// conversión a velocidad y guardar e[k] como e[k-1].
// posicionCarro [m] solo se usa con el lazo externo (LAZO_EXTERNO = 1).
void controlCalcular(float theta, float posicionCarro);
