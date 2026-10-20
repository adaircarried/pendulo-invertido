# Péndulo invertido lineal (ESP32 + stepper)

Control Avanzado, Unidad 2: PD digital diseñado por lugar de las raíces en z
(T = 10 ms, `u[k] = 635.94 e[k] − 584.27 e[k−1]`), implementado en un ESP32.
Todos los detalles del diseño y las reglas del proyecto están en [CLAUDE.md](CLAUDE.md).

## Archivos

```
platformio.ini            un entorno por fase
lib/config/config.h       pines, parámetros físicos y constantes del controlador
lib/encoder/              lectura del ángulo theta
lib/motor/                mando del stepper, finales de carrera y autohoming
lib/control/              ecuación en diferencias del controlador
src/fase1_encoder/        Fase 1: encoder en banco
src/fase2_stepper/        Fase 2: stepper en banco
src/control/              Fases 3, 4 y 5 (mismo programa, cambia FASE)
```

## Entornos

| Entorno | Fase | Mueve el motor |
|---|---|---|
| `fase1_encoder` | 1. Encoder en banco | No |
| `fase2_stepper` | 2. Aceleración máxima sin perder pasos | Sí |
| `fase3_control_sin_motor` | 3. Controlador sin motor | No |
| `fase4_signo` | 4. Verificación de signo (aceleración muy limitada) | Sí |
| `fase5_lazo_cerrado` | 5. Lazo cerrado (diseño de la clase) | Sí |
| `fase5_cascada` | 5. Lazo cerrado + lazo externo de posición (el carro regresa al centro) | Sí |

En VS Code se elige el entorno en la barra inferior y se usa ✓ (compilar), → (subir) y el enchufe (monitor).
Los comandos se escriben en el monitor y se mandan con Enter.

## Gráfica en tiempo real (Fases 3 a 5)

1. Instala la extensión **Teleplot** en VS Code.
2. Cierra el monitor serial de PlatformIO (el puerto solo puede abrirlo un programa a la vez).
3. Abre Teleplot, elige el puerto del ESP32, 115200 baudios, y "Open".
4. Aparecen las gráficas `theta` (grados), `aceleracion`, `velocidad` y `periodo_ms`.
   Los comandos (`a`, `p`, `h`...) se escriben en la caja de texto de Teleplot.

El comando `g` cambia entre gráfica y texto legible.

## Fase 1: encoder en banco

### Conexiones

| Cable del encoder | Conecta a |
|---|---|
| Rojo (VCC) | 5V / VIN del ESP32 |
| Negro (GND) | GND del ESP32 |
| Verde (A) | GPIO 32 |
| Blanco (B) | GPIO 33 |
| Malla (si tiene) | GND |

Sin resistencias: se usan las pull-ups internas (`PULLUP_INTERNA = true`).
No se conecta el driver ni el motor.

**Revisión antes de conectar A y B:** alimenta el encoder con A y B sueltos y mide de A a GND
y de B a GND girando el eje despacio. Debe marcar ~0 V o nada. **Si marca 5 V, no lo conectes.**

### Procedimiento

Encender con el péndulo (o el eje) colgando y quieto: esa posición es la cuenta 0.

| Prueba | Cómo | Debe dar |
|---|---|---|
| Cero | Encender colgando y quieto (o `z`) | cuentas 0, theta ±180° |
| Una vuelta | `m`, dar una vuelta exacta con una cinta como marca | 2400 cuentas = 1.000 vueltas |
| 90 grados | `m`, girar un cuarto de vuelta | 600 cuentas |
| Sin pérdidas | `m`, girar rápido de ida y vuelta muchas veces y regresar a la cinta | 0 cuentas desde la marca (±1) |
| Vertical | Girar media vuelta desde colgando | theta ≈ 0° |

## Conexiones completas (sistema montado)

### Resumen de pines del ESP32

| ESP32 | Va a |
|---|---|
| VIN (5V del USB) | Encoder rojo |
| 3.3V | Pull-ups del encoder, RST+SLP del driver, MS1/M0 y MS2/M1, (VDD si es A4988) |
| GND | Encoder negro, GND lógico del driver, GND de la fuente de 24 V, C de ambos finales, MS3/M2 |
| GPIO 32 | Encoder verde (A) + pull-up a 3.3V |
| GPIO 33 | Encoder blanco (B) + pull-up a 3.3V |
| GPIO 25 | STEP del driver |
| GPIO 26 | DIR del driver |
| GPIO 27 | EN del driver |
| GPIO 18 | NC del final izquierdo (solo si `USAR_FINALES_DE_CARRERA = true`) |
| GPIO 19 | NC del final derecho (solo si `USAR_FINALES_DE_CARRERA = true`) |

### Reglas

- **Tierra común:** GND del ESP32, GND del driver y negativo de la fuente de 24 V unidos.
- Los 24 V solo van a VMOT del driver. Nunca al ESP32.
- Capacitor de 100 µF (≥ 35 V) entre VMOT y GND del driver, lo más cerca posible, respetando polaridad.
- Nunca conectar ni desconectar el motor con el driver energizado.
- Pull-ups del encoder: cualquier valor entre 1k y 10k, siempre a **3.3V**. Menos de 1k no.
  Con externas puestas, `PULLUP_INTERNA = false` en `config.h`.
- Micropasos 1/8: los dos primeros pines de micropaso a 3.3V y el tercero a GND (igual en A4988 y DRV8825).
- Lado **derecho** = lado hacia el que theta es positivo (se determina con la Fase 1).

### Sin finales de carrera (configuración actual)

`USAR_FINALES_DE_CARRERA = false` en `config.h`. El carro se coloca a mano en el centro del riel y
el comando `h` toma esa posición como 0. El control se apaga si el carro se aleja más de
`RECORRIDO_UTIL / 2 − MARGEN_LIMITE` = 0.41 m del centro (límite por software con la cuenta de pasos).
Paro de emergencia: cortar los 24 V.

### Encendido y apagado

- Encender: primero USB (el programa deshabilita el driver), después la fuente de 24 V.
- Apagar: primero la fuente de 24 V, después el USB.
