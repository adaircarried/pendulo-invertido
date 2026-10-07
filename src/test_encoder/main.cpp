// test_encoder: lectura del encoder y validación de cuentas.
//
// Núcleo 0: cada Ts = 2 ms lee el encoder (igual que lo hará el control).
// Núcleo 1: imprime 10 veces por segundo y atiende comandos.
// El motor nunca da pasos; 'e' solo energiza el driver para probar el ruido.
//
// Comandos (monitor serial a 115200):
//   z  cero: péndulo colgando y quieto -> cuenta 0
//   m  marca la cuenta actual y reinicia el rango min..max
//   e  energiza / apaga el driver (sin pasos)

#include <Arduino.h>
#include <config.h>
#include <encoder.h>
#include <periodico.h>

// Lo que el núcleo 0 le manda al núcleo 1 en cada periodo
struct Muestra {
  LecturaEncoder enc;
  int32_t cuenta_min;  // con el péndulo quieto, min y max deben ser iguales (sin ruido)
  int32_t cuenta_max;
};

// Cola de un solo lugar: el núcleo 0 la sobreescribe, el núcleo 1 lee la última muestra
static QueueHandle_t buzon;

// Peticiones del núcleo 1 al núcleo 0
static volatile bool pedir_cero = false;
static volatile bool pedir_reinicio_rango = true;

static int32_t marca = 0;
static bool driver_encendido = false;

// ------------------------------------------------ Núcleo 0: cada 2 ms
// Aquí no va Serial ni delay.
static void tareaControl() {
  static int32_t cmin = 0, cmax = 0;

  if (pedir_cero) {
    encoderCero();
    pedir_cero = false;
    pedir_reinicio_rango = true;
  }

  Muestra m;
  m.enc = encoderLeer();

  if (pedir_reinicio_rango) {
    cmin = cmax = m.enc.cuentas;
    pedir_reinicio_rango = false;
  }
  cmin = min(cmin, m.enc.cuentas);
  cmax = max(cmax, m.enc.cuentas);
  m.cuenta_min = cmin;
  m.cuenta_max = cmax;

  xQueueOverwrite(buzon, &m);
}

// ------------------------------------------------ Núcleo 1
static void encenderDriver(bool on) {
  driver_encendido = on;
  digitalWrite(cfg::PIN_ENABLE, on ? LOW : HIGH);  // ENABLE es activo bajo
}

static void atenderComandos() {
  while (Serial.available()) {
    const char c = Serial.read();
    Muestra m;
    if (c == 'z') {
      pedir_cero = true;
      marca = 0;
      Serial.println("> cero (colgando)");
    } else if (c == 'm' && xQueuePeek(buzon, &m, 0) == pdTRUE) {
      marca = m.enc.cuentas;
      pedir_reinicio_rango = true;
      reiniciarPeriodo();
      Serial.printf("> marca = %ld\n", (long)marca);
    } else if (c == 'e') {
      encenderDriver(!driver_encendido);
      Serial.printf("> driver %s\n", driver_encendido ? "ENERGIZADO" : "apagado");
    }
  }
}

static void imprimir() {
  Muestra m;
  if (xQueuePeek(buzon, &m, 0) != pdTRUE) return;
  uint32_t pmin, pmax;
  leerPeriodo(pmin, pmax);

  const float vueltas = (float)(m.enc.cuentas - marca) / cfg::ENC_CUENTAS_VUELTA;
  Serial.printf("cuentas %6ld | vueltas %7.3f | theta %7.2f° | theta_dot %6.2f rad/s | "
                "rango %ld..%ld | Ts %lu..%lu us | driver %s\n",
                (long)m.enc.cuentas, vueltas, degrees(m.enc.theta), m.enc.theta_dot,
                (long)m.cuenta_min, (long)m.cuenta_max, (unsigned long)pmin,
                (unsigned long)pmax, driver_encendido ? "ON" : "off");
}

void setup() {
  // Seguridad primero: driver apagado y STEP en bajo.
  pinMode(cfg::PIN_ENABLE, OUTPUT);
  encenderDriver(false);
  pinMode(cfg::PIN_STEP, OUTPUT);
  digitalWrite(cfg::PIN_STEP, LOW);

  Serial.begin(115200);
  delay(200);

  buzon = xQueueCreate(1, sizeof(Muestra));
  encoderIniciar();
  iniciarTareaPeriodica(tareaControl, cfg::TS_US, cfg::NUCLEO_CONTROL, cfg::PRIORIDAD_CONTROL);

  Serial.println("\n=== test_encoder ===  z: cero  m: marca  e: driver on/off");
  Serial.printf("Pull-ups: %s\n", cfg::ENC_PULLUP_INTERNA ? "INTERNAS (solo pruebas)" : "externas 4.7k");
  Serial.println("Deja el péndulo colgando y quieto, y presiona 'z'.");
}

void loop() {
  atenderComandos();
  imprimir();
  delay(100);
}
