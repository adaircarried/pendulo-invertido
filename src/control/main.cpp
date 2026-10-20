// Programa de control: Fases 3, 4 y 5 (el entorno de platformio.ini define FASE).
//   FASE 3: controlador sin motor. Calcula e imprime la aceleración, pero no mueve nada.
//   FASE 4: verificación de signo, con aceleración y velocidad muy limitadas (config.h).
//   FASE 5: lazo cerrado completo.
//
// Núcleo 0: tareaControl(), cada T = 10 ms exactos.
// Núcleo 1: loop(), comandos y telemetría por serial (gráfica en tiempo real con Teleplot).
//
// Comandos (escribir y presionar Enter):
//   h  homing (Fases 4 y 5). Sin finales de carrera: coloca el carro a mano en el centro y presiona h
//   a  arrancar el control (péndulo levantado cerca de la vertical)
//   p  parar el control
//   z  cero del encoder (péndulo colgando y quieto, con el control parado)
//   g  alterna entre gráfica (Teleplot) y texto

#include <Arduino.h>

#include "config.h"
#include "control.h"
#include "encoder.h"
#include "motor.h"

// Motivos de paro del control
const int PARO_NINGUNO = 0;
const int PARO_COMANDO = 1;
const int PARO_THETA = 2;
const int PARO_LIMITE_CARRO = 3;

// ---------------------------------------------------- Variables compartidas entre núcleos
// El núcleo 1 solo "pide" (pedirArranque, pedirParo); el núcleo 0 es el único que actúa.
volatile bool pedirArranque = false;
volatile bool pedirParo = false;
volatile bool controlActivo = false;
volatile bool subiendo = false;  // swing-up en curso (solo fase6_swingup)
volatile int motivoParo = PARO_NINGUNO;

// Medición del periodo real de muestreo [us] (se reinician al arrancar el control)
volatile unsigned long periodoMinimoUs = 0;
volatile unsigned long periodoMaximoUs = 0;
volatile unsigned long ejecucionMaximaUs = 0;

// Última muestra del control, del núcleo 0 al núcleo 1 por una cola de un solo lugar
struct Muestra {
  float theta;        // [rad]
  float aceleracion;  // [m/s²]
  float velocidad;    // [m/s]
  float posicion;     // [m]
  float thetaRef;     // [rad]
  unsigned long periodoUs;
};
QueueHandle_t colaTelemetria;

bool modoGrafica = true;

// ---------------------------------------------------- Núcleo 0: ciclo de control
// Aquí no va Serial ni delay.

void pararControl(int motivo) {
  controlActivo = false;
  motorDetener();  // alto inmediato y driver deshabilitado (en la Fase 3 ya estaba deshabilitado)
  motivoParo = motivo;
}

void tareaControl(void* parametro) {
  const TickType_t periodoTicks = pdMS_TO_TICKS(lround(T * 1000));  // T = 10 ms (tick de 1 ms)
  TickType_t ultimoDespertar = xTaskGetTickCount();
  unsigned long inicioAnterior = micros();

  while (true) {
    // Dormir hasta exactamente 10 ms después del despertar anterior: periodo fijo
    vTaskDelayUntil(&ultimoDespertar, periodoTicks);

    unsigned long inicio = micros();
    unsigned long periodoUs = inicio - inicioAnterior;
    inicioAnterior = inicio;

    // 1) Leer theta
    float theta = encoderTheta();

    // Peticiones del núcleo 1
    if (pedirArranque) {
      if (SWING_UP && fabs(theta) > THETA_ARRANQUE) {
        swingupReiniciar(theta);  // empieza meciendo el péndulo
        subiendo = true;
      } else {
        controlReiniciar(theta);  // e[k-1] = e[k] y v = 0
        subiendo = false;
      }
      periodoMinimoUs = 1000000;
      periodoMaximoUs = 0;
      ejecucionMaximaUs = 0;
      motivoParo = PARO_NINGUNO;
      controlActivo = true;
      pedirArranque = false;
    }
    if (pedirParo) {
      pararControl(PARO_COMANDO);
      pedirParo = false;
    }

    if (controlActivo) {
      // Swing-up: al acercarse a la vertical, pasar al control de equilibrio
      if (subiendo && fabs(theta) < ANGULO_ATRAPE) {
        controlAtrapar(theta);
        subiendo = false;
      }

      // Seguridad (mientras sube, el ángulo grande es normal)
      if (!subiendo && fabs(theta) > THETA_MAXIMO) {
        pararControl(PARO_THETA);
      } else if (FASE >= 4 && (finalIzquierdoActivado() || finalDerechoActivado() || motorFueraDeLimites())) {
        pararControl(PARO_LIMITE_CARRO);
      } else {
        if (subiendo) {
          swingupCalcular(theta, motorPosicion());
        } else {
          // 2) a 4) error, ecuación en diferencias, saturación, velocidad y e[k-1]
          controlCalcular(theta, motorPosicion());
        }

        // 5) Mandar al motor (en la Fase 3 no se mueve)
        if (FASE >= 4) {
          motorVelocidad(velocidadComando, aceleracionComando);
        }
      }

      if (periodoUs < periodoMinimoUs) periodoMinimoUs = periodoUs;
      if (periodoUs > periodoMaximoUs) periodoMaximoUs = periodoUs;
    }

    // Telemetría para el núcleo 1
    Muestra muestra;
    muestra.theta = theta;
    muestra.aceleracion = aceleracionComando;
    muestra.velocidad = velocidadComando;
    muestra.posicion = motorPosicion();
    muestra.thetaRef = thetaReferencia;
    muestra.periodoUs = periodoUs;
    xQueueOverwrite(colaTelemetria, &muestra);

    unsigned long ejecucionUs = micros() - inicio;
    if (ejecucionUs > ejecucionMaximaUs) ejecucionMaximaUs = ejecucionUs;
  }
}

// ---------------------------------------------------- Núcleo 1: comandos y telemetría

void atenderComandos() {
  if (!Serial.available()) return;
  String linea = Serial.readStringUntil('\n');
  linea.trim();
  if (linea.length() == 0) return;
  char comando = linea[0];

  if (comando == 'a') {
    float theta = encoderTheta();
    if (controlActivo) {
      Serial.println("* el control ya está activo");
    } else if (FASE >= 4 && !motorHomingHecho()) {
      Serial.println("* primero haz el homing (h)");
    } else if (!SWING_UP && fabs(theta) > THETA_ARRANQUE) {
      Serial.printf("* levanta el péndulo: theta = %.1f°, debe ser menor a %.0f°\n",
                    theta * 180.0 / PI, THETA_ARRANQUE * 180.0 / PI);
    } else {
      pedirArranque = true;
      if (SWING_UP && fabs(theta) > THETA_ARRANQUE) {
        Serial.println("* SWING-UP: subiendo el péndulo");
      } else {
        Serial.println("* CONTROL ACTIVO");
      }
    }
  } else if (comando == 'p') {
    pedirParo = true;
  } else if (comando == 'h' && FASE >= 4) {
    if (controlActivo) {
      Serial.println("* para el control antes del homing");
    } else {
      Serial.println("* homing...");
      if (motorHoming()) {
        Serial.printf("* carro en el centro (posición 0). Recorrido: %.3f m\n", motorLargoRiel());
      }
    }
  } else if (comando == 'z') {
    if (controlActivo) {
      Serial.println("* para el control antes de tomar el cero");
    } else {
      encoderCero();
      Serial.println("* cero (colgando)");
    }
  } else if (comando == 'g') {
    modoGrafica = !modoGrafica;
  }
}

void reportarParo() {
  if (motivoParo == PARO_COMANDO) Serial.println("* CONTROL PARADO por comando");
  if (motivoParo == PARO_THETA) Serial.println("* CONTROL PARADO: |theta| > 30°, el péndulo cayó");
  if (motivoParo == PARO_LIMITE_CARRO) Serial.println("* CONTROL PARADO: el carro llegó al límite del riel");
  if (motivoParo != PARO_NINGUNO) {
    Serial.printf("* periodo medido: %lu..%lu us, ejecución máxima: %lu us\n",
                  periodoMinimoUs, periodoMaximoUs, ejecucionMaximaUs);
    if (FASE >= 4) Serial.println("* centra el carro y repite el homing (h) antes de arrancar de nuevo");
  }
  motivoParo = PARO_NINGUNO;
}

void imprimirTelemetria() {
  Muestra muestra;
  if (xQueuePeek(colaTelemetria, &muestra, 0) != pdTRUE) return;

  if (modoGrafica) {
    // Formato de Teleplot: ">nombre:valor", una gráfica por variable
    Serial.printf(">theta:%.2f\n", muestra.theta * 180.0 / PI);
    Serial.printf(">aceleracion:%.2f\n", muestra.aceleracion);
    Serial.printf(">velocidad:%.3f\n", muestra.velocidad);
    Serial.printf(">periodo_ms:%.2f\n", muestra.periodoUs / 1000.0);
    Serial.printf(">posicion:%.3f\n", muestra.posicion);
    if (LAZO_EXTERNO) Serial.printf(">theta_ref:%.2f\n", muestra.thetaRef * 180.0 / PI);
    if (SWING_UP) Serial.printf(">energia:%.2f\n", energiaPendulo);
  } else {
    Serial.printf("%s | theta %7.2f° | a %7.2f m/s² | v %6.3f m/s | x %6.3f m | periodo %lu us\n",
                  controlActivo ? "ACTIVO " : "parado ", muestra.theta * 180.0 / PI,
                  muestra.aceleracion, muestra.velocidad, muestra.posicion, muestra.periodoUs);
  }
}

void setup() {
  motorIniciar();  // primero: driver deshabilitado

  Serial.begin(115200);
  delay(200);

  encoderIniciar();  // el péndulo debe estar colgando y quieto al encender
  colaTelemetria = xQueueCreate(1, sizeof(Muestra));

  // Tarea de control en el núcleo 0 con prioridad alta (loop() corre en el núcleo 1)
  xTaskCreatePinnedToCore(tareaControl, "control", 4096, NULL, 20, NULL, 0);

  Serial.println();
  if (SWING_UP) {
    Serial.println("=== Fase 6: swing-up + equilibrio con lazo externo ===   h: homing   a: arrancar   p: parar   z: cero   g: gráfica/texto");
  } else {
    Serial.printf("=== Fase %d %s ===   h: homing   a: arrancar   p: parar   z: cero   g: gráfica/texto\n",
                  FASE, LAZO_EXTERNO ? "(con lazo externo de posición)" : "(diseño de la clase)");
  }
}

void loop() {
  atenderComandos();
  if (motivoParo != PARO_NINGUNO && !controlActivo) reportarParo();
  imprimirTelemetria();
  if (modoGrafica) {
    delay(20);   // 50 puntos por segundo en la gráfica
  } else {
    delay(200);  // 5 líneas por segundo en texto
  }
}
