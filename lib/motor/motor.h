// motor.h: mando del stepper (carro), finales de carrera y autohoming.
#pragma once

// Configura los pines y el generador de pulsos. Deja el driver deshabilitado.
void motorIniciar();

void motorHabilitar();
void motorDeshabilitar();

// Alto inmediato y driver deshabilitado (paro de emergencia). Obliga a repetir el homing.
void motorDetener();

// true si el final de carrera está presionado (o su cable cortado).
bool finalIzquierdoActivado();
bool finalDerechoActivado();

// Autohoming (bloqueante): busca el final izquierdo, luego el derecho, mide el riel
// y deja el carro en el centro, que queda como posición 0. Devuelve false si falla.
bool motorHoming();
bool motorHomingHecho();
float motorLargoRiel();  // [m], medido en el último homing

// Posición del carro [m] respecto al centro.
float motorPosicion();

// Lleva el carro a la posición x [m] con la velocidad y aceleración dadas (bloqueante).
// Si se activa un final de carrera, detiene todo y devuelve false.
bool motorMoverA(float x, float velocidad, float aceleracion);

// Mando del controlador, cada periodo T: llegar a la velocidad v [m/s] con
// aceleración constante |a| [m/s²] (así se reproduce el retenedor de orden cero).
void motorVelocidad(float v, float a);
