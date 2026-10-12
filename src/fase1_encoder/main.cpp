// Fase 1: encoder en banco (sin motor).
//
// Imprime cuentas y ángulo 10 veces por segundo. El contador de hardware cuenta solo,
// así que no hace falta una tarea de tiempo real: basta con leerlo en loop().
//
// Comandos por el monitor serial:
//   z  cero: péndulo colgando y quieto -> cuenta 0
//   m  marca la cuenta actual (para contar vueltas y comprobar que regresa al mismo valor)

#include <Arduino.h>

#include "config.h"
#include "encoder.h"

long marca = 0;

void setup() {
  // Seguridad: driver deshabilitado aunque esté conectado (ENABLE activo en bajo)
  pinMode(PIN_ENABLE, OUTPUT);
  digitalWrite(PIN_ENABLE, HIGH);

  Serial.begin(115200);
  delay(200);

  encoderIniciar();  // el péndulo debe estar colgando y quieto al encender

  Serial.println();
  Serial.println("=== Fase 1: encoder ===   z: cero   m: marca");
}

void loop() {
  // Comandos
  if (Serial.available()) {
    char comando = Serial.read();
    if (comando == 'z') {
      encoderCero();
      marca = 0;
      Serial.println("> cero (colgando)");
    } else if (comando == 'm') {
      marca = encoderCuentas();
      Serial.printf("> marca = %ld\n", marca);
    }
  }

  // Lectura
  long cuentas = encoderCuentas();
  float gradosDesdeColgando = cuentas * 360.0 / CUENTAS_POR_VUELTA;
  float theta = encoderTheta();
  float vueltasDesdeMarca = (float)(cuentas - marca) / CUENTAS_POR_VUELTA;

  Serial.printf("cuentas %7ld | desde colgando %8.2f° | theta %7.2f° | desde marca %6ld cuentas = %7.3f vueltas\n",
                cuentas, gradosDesdeColgando, theta * 180.0 / PI, cuentas - marca, vueltasDesdeMarca);

  delay(100);
}
