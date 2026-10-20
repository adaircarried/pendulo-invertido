# Actualización del proyecto: Péndulo invertido lineal (ESP32 + stepper)

**Esto no es un proyecto nuevo.** Ya trabajamos juntos con un `CLAUDE.md` anterior y ya existe código. Cambió el enfoque del control para alinearlo con la clase, y necesito que **reestructures el código existente** con las condiciones de este documento.

**Primera instrucción:** reemplaza el `CLAUDE.md` actual por este documento completo. A partir de ahora esta es la fuente de verdad. Si algún dato cambia durante el trabajo, actualízalo aquí.

## 0. Qué cambió respecto al CLAUDE.md anterior

| Antes | Ahora |
|---|---|
| PD por asignación de polos: Kp = 44.0, Kd = 4.79 | PD digital diseñado por lugar de las raíces en z: ecuación en diferencias con K = 635.94 y zc = 0.91875 |
| Ts = 2 ms (500 Hz) | **T = 10 ms (100 Hz)**, el del diseño |
| Derivada calculada aparte con filtro paso bajo | Sin derivada aparte: la acción derivativa ya está dentro de la ecuación en diferencias, mediante e[k-1] |
| Aceleración mandada directo con `moveByAcceleration` | Conversión explícita aceleración a velocidad, v[k] = v[k-1] + a[k]·T |
| Telemetría y cambio de ganancias por WiFi | Telemetría solo por serial; las ganancias son constantes del diseño, no se ajustan en vivo |
| Prioridad en aprovechar el ESP32 al máximo | **Prioridad en código sencillo y explicable** |

Lo que **se mantiene**: hardware, pines, encoder por PCNT, FastAccelStepper, separación en dos núcleos, reglas de seguridad y pruebas de banco.

---

## 1. Contexto

Proyecto de la materia Control Avanzado (Unidad 2: controladores digitales). Es la continuación de un péndulo invertido que el cuatrimestre pasado se controló con un PD analógico (amplificadores operacionales). Ahora el control es digital y se implementa en un ESP32.

El controlador **ya está diseñado en MATLAB** siguiendo el procedimiento de la clase:

1. Selección del tiempo de muestreo T.
2. Discretización de la planta con retenedor de orden cero: G(z) = (1 - z^-1) Z{G(s)/s}.
3. Diseño por lugar geométrico de las raíces en z (margen de K y K para el amortiguamiento deseado).
4. Ecuación en diferencias del controlador.
5. Comparación contra el control analógico.

**Tu trabajo es implementar en el ESP32 exactamente ese diseño.** No rediseñes el controlador, no cambies ganancias, no agregues filtros ni acciones que no estén en el diseño sin antes proponérmelo y explicar por qué.

## 2. Prioridad número uno: código sencillo y explicable

Tengo que poder explicarle cada línea a la maestra. Por eso:

- Framework Arduino en PlatformIO. Nada de ESP-IDF puro.
- Código en C++ sencillo, estilo Arduino: funciones normales, variables globales claras, ciclos y condicionales. Sin clases propias, plantillas, lambdas, punteros complicados ni memoria dinámica.
- Nombres de variables y funciones en español y descriptivos: `theta`, `errorActual`, `errorAnterior`, `aceleracionComando`.
- Cada constante lleva un comentario que diga de dónde sale (por ejemplo: "K del diseño por LGR en z, paso 3").
- La ecuación en diferencias debe verse en el código **tal como está en el reporte**, sin reacomodarla ni optimizarla.
- Comentarios breves: qué hace el bloque y a qué paso del diseño corresponde. Sin comentarios de relleno.
- Pocos archivos. Estructura sugerida:
  - `config.h`: pines, parámetros físicos y constantes del controlador.
  - `encoder.cpp/.h`: lectura del ángulo.
  - `motor.cpp/.h`: mando del stepper.
  - `control.cpp/.h`: ecuación en diferencias.
  - `main.cpp`: arranque, ciclo de control y telemetría.
- Si una librería esconde algo importante para entender el control, explícalo en un comentario o hazlo explícito en el código.
- Cuando termines cada fase, dame un resumen corto de qué hace cada archivo y cada función, para que yo lo pueda explicar.

## 3. Justificación del cambio de motor DC a stepper

El motor DC del cuatrimestre pasado era prestado y ya no está disponible. Se sustituyó por un motor a pasos NEMA 17.

- El motor DC impone una **fuerza** sobre el carro; su movimiento depende de la masa del carro y de la reacción del péndulo.
- El stepper impone directamente el **movimiento** del carro. Mientras no pierda pasos, el carro se mueve como se le ordena.

Por eso la entrada de la planta pasa de ser fuerza [N] a **aceleración del carro [m/s²]**. Partiendo de la misma ecuación linealizada del péndulo, l·θ'' = g·θ - a, se obtiene:

    Θ(s)/A(s) = -1 / (l s^2 - g)

Esta función es el caso límite del modelo anterior cuando m/M tiende a cero: es el mismo sistema físico, con un actuador que vuelve el movimiento del carro independiente de la reacción del péndulo. Conserva el orden y el polo inestable, por lo que la metodología de diseño se mantiene.

**Supuesto clave:** el modelo es válido mientras el stepper no pierda pasos. Por eso la aceleración comandada siempre se satura al máximo medido en la prueba de banco.

## 4. Datos del sistema

| Elemento | Valor |
|---|---|
| Microcontrolador | ESP32 clásico: placa NodeMCU-32S (módulo ESP-32S). En PlatformIO se usa `board = esp32dev` (equivalente) |
| Actuador | NEMA 17 **17HS4401** (1.7 A nominal) sin reducción, driver **DRV8825** (Vref ≈ 0.65 V → 1.3 A), micropasos 1/8 (M0 = 3.3 V, M1 = 3.3 V, M2 = GND) |
| Transmisión | Banda GT2, polea de 20 dientes (40 mm por vuelta) |
| Conversión | 1600 pasos por vuelta, **40,000 pasos por metro** |
| Sensor | Encoder incremental LPD3806-600BM (NPN colector abierto) |
| Resolución | 600 PPR x 4 = **2400 cuentas por vuelta** (0.15 grados por cuenta) |
| Montaje | Péndulo directo al eje del encoder |
| g | 9.81 m/s² |
| l (pivote a centro de masa) | 0.342 m |

### Pines

| Señal | GPIO |
|---|---|
| Encoder A (verde) | 32 |
| Encoder B (blanco) | 33 |
| STEP | 25 |
| DIR | 26 |
| ENABLE (activo en bajo) | 27 |
| Final de carrera izquierdo | 18 |
| Final de carrera derecho | 19 |

Pull-ups del encoder a 3.3 V (externas o internas del ESP32). Nunca a 5 V.
En la mesa se usan las internas (`PULLUP_INTERNA = true`); en el montaje final, externas de 1k a 10k (`false`).

Finales de carrera normalmente cerrados (NC) entre el pin y GND, con pull-up interna:
en reposo leen LOW; presionados o con el cable cortado leen HIGH (falla segura).

**Por ahora sin finales de carrera** (`USAR_FINALES_DE_CARRERA = false`): el carro se centra a mano,
`h` toma esa posición como 0 y el control se apaga a 0.41 m del centro (recorrido útil medido 0.88 m,
margen 3 cm). Por ahora pull-ups internas del encoder (`PULLUP_INTERNA = true`): no hay resistencias de 1k a 10k.

## 5. Retroalimentación (sensor)

- La variable medida y controlada es el **ángulo θ del péndulo**, leído con el encoder.
- Lectura por el contador de pulsos de hardware del ESP32 (librería ESP32Encoder), con su filtro de hardware activado. Esto evita perder cuentas aunque el procesador esté ocupado.
- **Referencia del cero:** al encender, el péndulo está colgando y quieto. Esa posición se toma como referencia. La vertical hacia arriba es esa posición más media vuelta (1200 cuentas). Así el cero no depende de alinearlo a ojo.
- Convención: θ = 0 en la vertical arriba, en radianes, positivo hacia el mismo lado en que el carro avanza con aceleración positiva. El signo se verifica experimentalmente en la Fase 4.
- La referencia del controlador es θ = 0 (regulación).
- Dato a tener presente: una cuenta del encoder equivale a 0.0026 rad. En la ecuación en diferencias, un cambio de una cuenta mueve la aceleración comandada unos 1.7 m/s². Si eso genera vibración, repórtamelo antes de agregar cualquier filtro.

## 6. Controlador (resultado del diseño en MATLAB)

- Tiempo de muestreo: **T = 0.01 s (100 Hz)**.
- Planta discreta (ZOH): G(z) = 1.4623e-4 (z + 1) / (z^2 - 2.002869 z + 1).
- Controlador PD digital: D(z) = K (z - zc) / z, con **K = 635.94** y **zc = 0.91875**.
- Margen de estabilidad: 121 < K < 6,822.
- Ecuación en diferencias (con e[k] = 0 - θ[k]):

      u[k] = 635.94 e[k] - 584.27 e[k-1]

- Como el signo de la planta se absorbió en el diseño, la aceleración física del carro es a[k] = -u[k], es decir:

      a[k] = 635.94 θ[k] - 584.27 θ[k-1]     [m/s²]

- Aceleración pico esperada ante una perturbación de 10 grados: aproximadamente 9.1 m/s².

Escribe el controlador en el código como e[k], u[k] y luego a = -u, para que coincida con el reporte.

## 7. Cómo se manda la aceleración al stepper

El controlador entrega aceleración, pero el motor se maneja por velocidad. La conversión debe ser explícita en el código:

    v[k] = v[k-1] + a[k] · T

Después se convierte a pasos por segundo (multiplicando por 40,000) y se le manda al motor con FastAccelStepper, que genera los pulsos de STEP por hardware. Satura la aceleración y la velocidad a los máximos medidos en la Fase 2.

## 8. Estructura del programa

- **Ciclo de control** en una tarea propia, fijada a un núcleo, que se ejecuta cada 10 ms exactos (`vTaskDelayUntil`). Hace solo esto, en orden: leer θ, calcular el error, aplicar la ecuación en diferencias, saturar, mandar al motor, guardar e[k] como e[k-1]. Nada de `Serial` ni `delay` aquí.
- **Telemetría** en `loop()`, en el otro núcleo: imprime θ, a y velocidad por serial, y recibe comandos simples (arrancar, parar, cero, homing).
  Por defecto en formato de Teleplot (`>theta:valor`) para ver una gráfica de θ en tiempo real en VS Code y ver cuándo se estabiliza en 0.
- Mide y reporta el tiempo real de ejecución del ciclo para comprobar que el muestreo es constante. Si el periodo no es estable, el diseño discreto deja de ser válido.

## 9. Seguridad (obligatoria en todo programa que mueva el motor)

- Saturar aceleración y velocidad.
- Si se activa un final de carrera: detener y deshabilitar el driver de inmediato.
- Si |θ| > 30 grados: apagar el control (el péndulo ya cayó).
- El control solo arranca por comando, con el péndulo levantado cerca de la vertical (|θ| < 5°) y con el homing hecho.
- **Autohoming** (Fases 2, 4 y 5): busca despacio el final izquierdo, luego el derecho, mide el riel y deja el carro al centro (posición 0).
  Si se activa el final contrario al buscado, aborta (motor invertido o finales cruzados). Cualquier paro de emergencia obliga a repetir el homing.

## 10. Plan de trabajo por fases

Cada fase es un entorno separado en `platformio.ini` y reutiliza los mismos módulos que el programa final. **No avances a la siguiente fase sin que yo confirme que la anterior pasó sus criterios.**

**Fase 1. Encoder en banco** (sin motor)
- Imprimir cuentas y ángulo.
- Criterios: una vuelta = 2400 cuentas; 90 grados = 600; al regresar a la posición inicial, la cuenta vuelve al mismo valor.

**Fase 2. Stepper en banco** (sin péndulo)
- Ciclos de ida y vuelta con aceleración configurable por serial.
- Criterio: encontrar la aceleración máxima sin perder pasos (el carro regresa exacto a su marca). Meta: 15 m/s² o más, para tener margen sobre los 9.1 m/s² del diseño.

**Fase 3. Controlador sin motor**
- Corre la ecuación en diferencias cada 10 ms con el encoder real, pero sin mover el motor: solo imprime θ y la aceleración que mandaría.
- Criterios: el periodo medido es 10 ms; al inclinar el péndulo, la aceleración cambia de signo y su magnitud es coherente con la ecuación.

**Fase 4. Verificación de signo** (sistema montado, carro sujeto con la mano)
- Con el motor habilitado y aceleración muy limitada.
- Criterio: al inclinar el péndulo, el carro empuja hacia el lado al que cae. Si empuja al contrario, se invierte el sentido del motor en `config.h`, no la ecuación.

**Fase 5. Lazo cerrado**
- Control completo con todas las protecciones.
- Criterio: el péndulo se mantiene en la vertical. Se registra θ por serial para compararlo contra la simulación de MATLAB.

**Pendiente conocido (después de la Fase 5):** el control solo de ángulo deja al carro con una deriva lenta. La simulación muestra unos 0.7 m en 3 s. Si en las pruebas el carro llega a los extremos, lo resolveremos con un lazo externo de posición usando la cuenta de pasos. No lo implementes todavía.

## 11. Decisiones de implementación (acordadas)

- **Condición inicial:** al arrancar el control, e[k-1] = e[k] y v = 0. Si e[k-1] fuera 0, la primera muestra pediría ~111 m/s² con el péndulo a 10°; así pide los 9.0 m/s² del diseño.
- **Mando al motor:** en cada periodo se le pide a FastAccelStepper llegar a v[k] con aceleración constante |a[k]|. La velocidad sube en rampa durante T, que es el retenedor de orden cero del diseño.
- **Una sola perilla de signo:** `MOTOR_INVERTIDO` en `config.h`. No hay inversión de signo del encoder.
- **Fases 3, 4 y 5 son un solo programa** (`src/control/main.cpp`); el entorno define `FASE`. La Fase 4 usa saturaciones muy bajas.
- **Periodo:** `vTaskDelayUntil` con el tick de FreeRTOS de 1 ms (10 ticks = 10 ms).

## 12. Estado actual

- Reestructuración terminada. Todos los entornos compilan en PlatformIO (confirmado por el autor).
- **Fase 1 aprobada:** 1 vuelta = 2400 cuentas, 90° = 600, regreso a la marca = 0, vertical = 0°. Lado DER (+) marcado.
- Vref del DRV8825 ajustado a 0.66 V (≈ 1.3 A).
- Fase 2: STEP y DIR estaban cruzados (corregido). Con DIR bien conectado el motor avanzaba hacia IZQ: `MOTOR_INVERTIDO = true`.
- **Fase 2:** polea floja corregida (apretar prisioneros sobre la cara plana del eje). Recorrido correcto (15 cm = 6000 pasos).
  Sin perder pasos hasta **7 m/s²** a 0.5 m/s; con 8 m/s² pierde pasos. No se alcanzó la meta de 15 m/s².
  El driver se calienta mucho: necesita disipador y ventilador.
- Saturaciones de la Fase 5: **5.5 m/s² y 0.4 m/s** (~80 % de lo medido). Con a = 635.94 θ[k] − 584.27 θ[k−1],
  la saturación se alcanza a partir de ~6° (≈ 0.9 m/s² por grado). Simulación no lineal con estas saturaciones:
  estable desde 2°, 4°, 6°, 8° y 10° (modelo ideal, sin pérdida de pasos ni fricción).
- **Fase 2 aprobada** con máximo de 7 m/s² (aceptado por el autor).
- **Fase 3 aprobada:** periodo 10 ms constante; con el péndulo quieto en ~3° la aceleración fue 2.71 m/s²
  (= 51.67·θ, porque con θ[k] = θ[k−1] la ecuación queda a = (635.94 − 584.27)·θ). Paro a más de 30° correcto.
- Fase 4: al arrancar el motor, theta saltaba ~−74°. Causa: ESP32Encoder y FastAccelStepper (modo MCPWM+PCNT)
  usaban el mismo contador PCNT 0. Solución: `stepperConnectToPin(PIN_STEP, DRIVER_RMT)`.
- **Fase 4 aprobada:** con `MOTOR_INVERTIDO = true` el carro va hacia el lado al que cae el péndulo (DER y IZQ); theta estable al arrancar.
- **Fase 5:** el péndulo se equilibra hasta que la deriva lleva el carro al límite del riel. Sin vibración, periodo 10 ms plano, driver aguanta.
- Siguiente: lazo externo de posición (propuesto al autor, pendiente de aprobación).
- Sin finales de carrera: centrado manual y límite por software a ±0.41 m (margen 3 cm).
- Lado DER (+) = lado hacia el que theta es positivo (Fase 1); el motor debe avanzar hacia ahí (se ve en la Fase 2).
- Deriva esperada (simulación con el límite de ±0.39 m): desde 2° el carro llega al límite en ~8 s,
  desde 5° en ~3 s y desde 10° en ~1.5 s. El lazo externo de posición es el pendiente de la sección 10.

## 13. Cómo se hizo la reestructuración (completado)

1. Reemplaza el `CLAUDE.md` por este documento.
2. **Antes de modificar nada**, revisa todo el código existente y dame un diagnóstico corto:
   - Qué archivos y entornos existen.
   - Qué se conserva tal cual, qué se modifica y qué se elimina, con la razón de cada uno según la tabla de la sección 0.
   - En qué fase del plan quedó el proyecto.
3. Espera mi confirmación del diagnóstico.
4. Reestructura **reutilizando lo que ya funciona**. No reescribas desde cero un módulo que solo necesita ajustes. Si una prueba ya fue validada en hardware (por ejemplo la del encoder), no cambies su comportamiento; solo adapta su estilo y nombres a las reglas de la sección 2.
5. Elimina por completo lo que ya no aplica: las ganancias Kp = 44 y Kd = 4.79, el filtro de la derivada, el periodo de 2 ms y cualquier ajuste de ganancias en vivo. Que no queden restos comentados.
6. Verifica que todos los entornos compilen.
7. Al terminar, dame el resumen de qué hace cada archivo y cada función, y dime desde qué fase del plan debo continuar las pruebas.
