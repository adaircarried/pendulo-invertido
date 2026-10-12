// motor.cpp: mando del stepper con FastAccelStepper.
//
// FastAccelStepper genera los pulsos de STEP por hardware: nosotros solo le decimos a qué
// velocidad (pasos/s) y con qué aceleración (pasos/s²) moverse, y él cuenta cada paso dado.
// Toda conversión de metros a pasos usa PASOS_POR_METRO = 40,000.

#include "motor.h"

#include <FastAccelStepper.h>

#include "config.h"

static FastAccelStepperEngine generador;
static FastAccelStepper* stepper = NULL;  // la librería entrega el motor como puntero: se usa con ->

static bool homingHecho = false;
static long largoRielPasos = 0;

// Por debajo de esta velocidad se considera que el carro debe estar detenido [pasos/s]
const long VELOCIDAD_MINIMA_PASOS = 10;

static long metrosAPasos(float metros) {
  return lround(metros * PASOS_POR_METRO);
}

void motorIniciar() {
  // Primero el driver deshabilitado, antes que cualquier otra cosa
  pinMode(PIN_ENABLE, OUTPUT);
  digitalWrite(PIN_ENABLE, HIGH);

  pinMode(PIN_FINAL_IZQ, INPUT_PULLUP);
  pinMode(PIN_FINAL_DER, INPUT_PULLUP);

  generador.init();
  stepper = generador.stepperConnectToPin(PIN_STEP);
  stepper->setDirectionPin(PIN_DIR, !MOTOR_INVERTIDO);  // MOTOR_INVERTIDO voltea el sentido
  stepper->setEnablePin(PIN_ENABLE, true);              // true: ENABLE activo en bajo
  stepper->setAutoEnable(false);                        // habilitar/deshabilitar lo decidimos nosotros
  stepper->disableOutputs();
}

void motorHabilitar() {
  stepper->enableOutputs();
}

void motorDeshabilitar() {
  stepper->disableOutputs();
}

void motorDetener() {
  stepper->forceStop();      // deja de generar pasos
  stepper->disableOutputs(); // y corta la corriente del motor
  homingHecho = false;       // sin corriente el carro pudo moverse: hay que repetir el homing
}

bool finalIzquierdoActivado() {
  return digitalRead(PIN_FINAL_IZQ) == NIVEL_FINAL_ACTIVADO;
}

bool finalDerechoActivado() {
  return digitalRead(PIN_FINAL_DER) == NIVEL_FINAL_ACTIVADO;
}

// Mueve el carro despacio hacia un lado hasta tocar ese final de carrera.
// Aborta si se tarda demasiado o si se activa el final del lado contrario
// (eso indica que el motor gira al revés o que los finales están intercambiados).
static bool irHastaFinal(bool haciaIzquierda) {
  bool otroFinalAlInicio;
  if (haciaIzquierda) {
    otroFinalAlInicio = finalDerechoActivado();
  } else {
    otroFinalAlInicio = finalIzquierdoActivado();
  }

  stepper->setSpeedInHz(metrosAPasos(VELOCIDAD_HOMING));
  stepper->setAcceleration(metrosAPasos(ACELERACION_HOMING));
  if (haciaIzquierda) {
    stepper->runBackward();
  } else {
    stepper->runForward();
  }

  unsigned long inicio = millis();
  while (true) {
    bool finalBuscado, otroFinal;
    if (haciaIzquierda) {
      finalBuscado = finalIzquierdoActivado();
      otroFinal = finalDerechoActivado();
    } else {
      finalBuscado = finalDerechoActivado();
      otroFinal = finalIzquierdoActivado();
    }

    if (finalBuscado) break;
    if (otroFinal && !otroFinalAlInicio) {
      motorDetener();
      Serial.println("ERROR homing: se activó el final contrario (¿MOTOR_INVERTIDO o finales cruzados?)");
      return false;
    }
    if (millis() - inicio > TIEMPO_MAX_HOMING_MS) {
      motorDetener();
      Serial.println("ERROR homing: no se encontró el final de carrera");
      return false;
    }
    delay(1);
  }

  stepper->forceStop();
  while (stepper->isRunning()) delay(1);
  return true;
}

bool motorHoming() {
  homingHecho = false;
  motorHabilitar();

  // 1) Final izquierdo: ahí se pone la cuenta de pasos en 0
  if (!irHastaFinal(true)) return false;
  stepper->setCurrentPosition(0);
  stepper->move(metrosAPasos(RETROCESO_HOMING), true);  // se separa del final (true = esperar)

  // 2) Final derecho: los pasos contados hasta ahí son el largo del riel
  if (!irHastaFinal(false)) return false;
  largoRielPasos = stepper->getCurrentPosition();

  // 3) Ir al centro, que pasa a ser la posición 0
  stepper->moveTo(largoRielPasos / 2, true);
  stepper->setCurrentPosition(0);

  homingHecho = true;
  return true;
}

bool motorHomingHecho() {
  return homingHecho;
}

float motorLargoRiel() {
  return largoRielPasos / PASOS_POR_METRO;
}

float motorPosicion() {
  return stepper->getCurrentPosition() / PASOS_POR_METRO;
}

bool motorMoverA(float x, float velocidad, float aceleracion) {
  stepper->setSpeedInHz(metrosAPasos(velocidad));
  stepper->setAcceleration(metrosAPasos(aceleracion));
  stepper->moveTo(metrosAPasos(x));

  while (stepper->isRunning()) {
    if (finalIzquierdoActivado() || finalDerechoActivado()) {
      motorDetener();
      return false;
    }
    delay(1);
  }
  return true;
}

void motorVelocidad(float v, float a) {
  long velocidadPasos = metrosAPasos(fabs(v));
  long aceleracionPasos = metrosAPasos(fabs(a));
  if (aceleracionPasos < 1) aceleracionPasos = 1;  // la librería no acepta aceleración 0

  // Rampa de aceleración constante |a| durante el periodo: así el carro sigue el ZOH del diseño
  stepper->setAcceleration(aceleracionPasos);

  if (velocidadPasos < VELOCIDAD_MINIMA_PASOS) {
    stepper->stopMove();  // frena hasta 0 con la aceleración |a|
    return;
  }
  stepper->setSpeedInHz(velocidadPasos);
  if (v > 0) {
    stepper->runForward();
  } else {
    stepper->runBackward();
  }
}
