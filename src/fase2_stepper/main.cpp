// Fase 2: stepper en banco (carro en el riel, sin péndulo).
//
// Objetivo: encontrar la aceleración máxima sin perder pasos.
// Se hacen ciclos de ida y vuelta alrededor del centro; al terminar, el carro regresa
// al centro. Si quedó exactamente en su marca, no perdió pasos.
//
// Comandos (escribir y presionar Enter):
//   h        homing. Sin finales de carrera: coloca el carro a mano en el centro y presiona h
//   a15      aceleración de prueba = 15 m/s²
//   v0.5     velocidad de prueba = 0.5 m/s
//   c        hace los ciclos de ida y vuelta
//   p        detiene y deshabilita el motor

#include <Arduino.h>

#include "config.h"
#include "motor.h"

const int NUMERO_DE_CICLOS = 5;
const float DISTANCIA_PRUEBA = 0.15;  // [m] a cada lado del centro

float aceleracionPrueba = 2.0;  // [m/s²]
float velocidadPrueba = 0.5;    // [m/s]

void hacerCiclos() {
  if (!motorHomingHecho()) {
    Serial.println("Primero haz el homing (h).");
    return;
  }

  // No acercarse a menos de MARGEN_LIMITE de los extremos del recorrido
  float distancia = min(DISTANCIA_PRUEBA, motorLargoRiel() / 2 - MARGEN_LIMITE);

  Serial.printf("> %d ciclos de ±%.3f m con a = %.2f m/s² y v = %.2f m/s\n",
                NUMERO_DE_CICLOS, distancia, aceleracionPrueba, velocidadPrueba);

  Serial.println("> El primer movimiento va hacia DER (+). Si va hacia IZQ: corta los 24 V y pon MOTOR_INVERTIDO = true.");
  delay(2000);

  for (int i = 0; i < NUMERO_DE_CICLOS; i++) {
    if (!motorMoverA(distancia, velocidadPrueba, aceleracionPrueba) ||
        !motorMoverA(-distancia, velocidadPrueba, aceleracionPrueba)) {
      Serial.println("PARO: se activó un final de carrera. Repite el homing.");
      return;
    }
  }
  motorMoverA(0, velocidadPrueba, aceleracionPrueba);
  Serial.println("> Listo. ¿El carro quedó exactamente en su marca? Si sí, no perdió pasos.");
}

void setup() {
  motorIniciar();  // deja el driver deshabilitado

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=== Fase 2: stepper ===   h: homing   a<valor>   v<valor>   c: ciclos   p: parar");
}

void loop() {
  if (Serial.available()) {
    String linea = Serial.readStringUntil('\n');
    linea.trim();
    if (linea.length() == 0) return;

    char comando = linea[0];
    float valor = linea.substring(1).toFloat();

    if (comando == 'h') {
      Serial.println("> homing...");
      if (motorHoming()) {
        Serial.printf("> carro en el centro (posición 0). Recorrido: %.3f m\n", motorLargoRiel());
        Serial.println("> Haz una marca en el riel junto al carro.");
      }
    } else if (comando == 'a' && valor > 0) {
      aceleracionPrueba = valor;
      Serial.printf("> aceleración de prueba = %.2f m/s²\n", aceleracionPrueba);
    } else if (comando == 'v' && valor > 0) {
      velocidadPrueba = valor;
      Serial.printf("> velocidad de prueba = %.2f m/s\n", velocidadPrueba);
    } else if (comando == 'c') {
      hacerCiclos();
    } else if (comando == 'p') {
      motorDetener();
      Serial.println("> motor detenido y deshabilitado (repite el homing antes de seguir)");
    }
  }
}
