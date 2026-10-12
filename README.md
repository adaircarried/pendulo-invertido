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
| `fase5_lazo_cerrado` | 5. Lazo cerrado | Sí |

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
