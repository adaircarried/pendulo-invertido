// encoder.h: lectura del ángulo del péndulo (retroalimentación).
#pragma once

// Configura el contador de pulsos de hardware y toma la posición actual como cero.
// Al llamarla, el péndulo debe estar colgando y quieto.
void encoderIniciar();

// Vuelve a tomar la posición actual como cero (péndulo colgando y quieto).
void encoderCero();

// Cuentas acumuladas desde el cero (2400 por vuelta).
long encoderCuentas();

// Ángulo theta [rad]: 0 en la vertical arriba, entre -pi y +pi.
float encoderTheta();
