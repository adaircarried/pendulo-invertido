# Péndulo invertido lineal (ESP32 + stepper)

Péndulo invertido sobre carro en riel V-Slot, estabilizado con un PD discreto en un ESP32.
Detalles de hardware, modelo y reglas del proyecto en [CLAUDE.md](CLAUDE.md).

## Estructura

```
platformio.ini          entornos test_encoder, test_stepper, main
lib/
  config/config.h       pines y constantes (única fuente de verdad)
  encoder/              lectura del encoder: cuentas -> theta, theta_dot
  periodico/            tarea de tiempo real cada Ts, disparada por timer de hardware
src/
  test_encoder/         prueba 1: validación del encoder
  test_stepper/         prueba 2: aceleración máxima sin pérdida de pasos (pendiente)
  main/                 control completo (pendiente)
```

Cada entorno compila solo su carpeta de `src/` y usa los mismos módulos de `lib/`.

```bash
pio run -e test_encoder -t upload -t monitor
```

## Prueba 1: encoder (`test_encoder`)

### Material

- ESP32 (DevKit / esp32dev) y cable USB
- Encoder LPD3806-600BM
- Protoboard y jumpers
- Multímetro
- Para el montaje final: 2 resistencias de 4.7 kΩ (en la mesa se usan las pull-ups internas)

### Conexiones (versión de mesa, pull-ups internas)

| Cable del encoder | Conecta a |
|---|---|
| Rojo (VCC) | **5V / VIN** del ESP32 |
| Negro (GND) | **GND** del ESP32 |
| Verde (A) | **GPIO 32** |
| Blanco (B) | **GPIO 33** |
| Malla (si tiene) | GND |

No hace falta conectar el driver ni el motor para los pasos 1-5.

**Antes de conectar A y B al ESP32:** alimenta el encoder (rojo a 5V, negro a GND) con A y B
sueltos y mide con el multímetro de A a GND y de B a GND, girando el eje despacio.
Debe marcar ~0 V o nada (salida de colector abierto). **Si marca 5 V, no lo conectes**:
el encoder tiene pull-ups internas a 5 V y quemaría el ESP32.

### Conexiones (versión final, pull-ups externas)

Igual que arriba, más una resistencia de 4.7 kΩ de GPIO 32 a **3.3V** y otra de GPIO 33 a **3.3V**
(nunca a 5V). En `lib/config/config.h` cambiar `ENC_PULLUP_INTERNA` a `false`.

### Comandos (monitor serial a 115200)

| Tecla | Acción |
|---|---|
| `z` | cero: péndulo colgando y quieto → cuenta 0 |
| `m` | marca la cuenta actual y reinicia el rango min..max y el periodo |
| `e` | energiza / apaga el driver (sin pasos) |

### Procedimiento

1. **Cero:** péndulo (o eje) quieto, `z`. Cuenta = 0, theta = ±180°.
2. **Cuentas por vuelta:** pega una cinta en el eje como marca, `m`, gira exactamente 5 vueltas.
   "vueltas" debe marcar 5.000 (12000 cuentas). Repite en el otro sentido: −5.000.
3. **Sin pérdidas:** gira rápido de ida y vuelta muchas veces y regresa a la cinta: vueltas ≈ 0.000.
4. **Periodo:** `Ts` debe quedar cerca de 2000..2000 us (unos pocos us de diferencia).
5. **Ruido:** eje quieto, `m`, espera un minuto: el rango min..max debe ser un solo valor.
6. **Ruido con motor (requiere driver y motor conectados):** `e`, `m`, espera unos minutos.
   Con pull-ups internas esta prueba es más exigente; si aparece ruido, repetir con las de 4.7 kΩ.
7. **Signo (requiere el péndulo montado en el carro):** súbelo a la vertical (theta ≈ 0°) e inclínalo
   hacia donde avanza el carro con `a > 0`: theta debe ser positivo. Si no, `ENC_SIGNO = -1`.
