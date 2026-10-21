from reportlab.lib.pagesizes import letter
from reportlab.platypus import (PageBreak, SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
                                Image, Preformatted, KeepTogether)
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import cm
from reportlab.lib import colors
from reportlab.lib.enums import TA_JUSTIFY, TA_CENTER
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
import sys

F = "/usr/share/fonts/truetype/dejavu/"
pdfmetrics.registerFont(TTFont("DV", F + "DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("DVB", F + "DejaVuSans-Bold.ttf"))
pdfmetrics.registerFont(TTFont("DVI", F + "DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("DVM", F + "DejaVuSansMono.ttf"))
from reportlab.lib.fonts import addMapping
addMapping("DV", 0, 0, "DV"); addMapping("DV", 1, 0, "DVB"); addMapping("DV", 0, 1, "DVI"); addMapping("DV", 1, 1, "DVB")

SALIDA = sys.argv[1]
IMAGEN = sys.argv[2]
LOGO_UPA = sys.argv[3]
LOGO_MECA = sys.argv[4]
ALUMNO = sys.argv[5] if len(sys.argv) > 5 else "[nombre del alumno]"
AZUL = colors.HexColor("#1F3A5F")
GRIS = colors.HexColor("#EEF1F5")

cuerpo = ParagraphStyle("c", fontName="DV", fontSize=9.6, leading=13.6, alignment=TA_JUSTIFY, spaceAfter=5)
titulo = ParagraphStyle("t", fontName="DVB", fontSize=15, leading=19, alignment=TA_CENTER, textColor=AZUL, spaceAfter=4)
sub = ParagraphStyle("s", fontName="DV", fontSize=10, leading=14, alignment=TA_CENTER, spaceAfter=2)
h1 = ParagraphStyle("h1", fontName="DVB", fontSize=11.5, leading=15, textColor=AZUL, spaceBefore=9, spaceAfter=4)
ecu = ParagraphStyle("e", fontName="DV", fontSize=10, leading=15, alignment=TA_CENTER, spaceBefore=2, spaceAfter=6)
celda = ParagraphStyle("cel", fontName="DV", fontSize=8.6, leading=11)
celdaB = ParagraphStyle("celb", parent=celda, fontName="DVB")
pie = ParagraphStyle("pie", fontName="DVI", fontSize=8.6, leading=11, alignment=TA_CENTER, spaceAfter=6)
codigo = ParagraphStyle("cod", fontName="DVM", fontSize=7.9, leading=10.2)


def P(t): return Paragraph(t, cuerpo)
def H(t): return Paragraph(t, h1)
def E(t): return Paragraph(t, ecu)


def tabla(filas, anchos):
    datos = [[Paragraph(c, celdaB if i == 0 else celda) for c in f] for i, f in enumerate(filas)]
    t = Table(datos, colWidths=anchos, repeatRows=1)
    t.setStyle(TableStyle([
        ("LINEABOVE", (0, 0), (-1, 0), 1.0, colors.black),
        ("LINEBELOW", (0, 0), (-1, 0), 0.6, colors.black),
        ("LINEBELOW", (0, -1), (-1, -1), 1.0, colors.black),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 4),
        ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
    ]))
    return KeepTogether([t])


h = []
# ---------------- Portada
logos = Table([[Image(LOGO_UPA, width=5.6 * cm, height=5.6 * cm * 600 / 801),
                Image(LOGO_MECA, width=7.4 * cm, height=7.4 * cm * 281 / 889)]], colWidths=[8.3 * cm, 8.3 * cm])
logos.setStyle(TableStyle([("ALIGN", (0, 0), (-1, -1), "CENTER"), ("VALIGN", (0, 0), (-1, -1), "MIDDLE")]))
portadaTitulo = ParagraphStyle("pt", parent=titulo, fontSize=21, leading=28)
portadaDato = ParagraphStyle("pd", parent=sub, fontSize=12, leading=19)
portadaEtiqueta = ParagraphStyle("pe", parent=sub, fontName="DVB", fontSize=12, leading=19, textColor=AZUL)
h.append(logos)
h.append(Spacer(1, 2.6 * cm))
h.append(Paragraph("Control digital de un péndulo<br/>invertido lineal con ESP32<br/>y motor a pasos", portadaTitulo))
h.append(Spacer(1, 0.5 * cm))
h.append(Paragraph("Evidencia de producto · Unidad 2: Controladores digitales", portadaDato))
h.append(Spacer(1, 2.2 * cm))
for etiqueta, dato in (("Materia", "Control Avanzado"), ("Carrera", "Ingeniería Mecatrónica"),
                       ("Grupo", "MTR09A"), ("Equipo", "8"), ("Alumno", ALUMNO)):
    h.append(Paragraph(etiqueta, portadaEtiqueta))
    h.append(Paragraph(dato, portadaDato))
    h.append(Spacer(1, 0.15 * cm))
h.append(Spacer(1, 1.2 * cm))
h.append(Paragraph("Aguascalientes, Ags. · Octubre de 2026", portadaDato))
h.append(PageBreak())

h.append(H("1. Objetivo"))
h.append(P("Estabilizar un péndulo invertido sobre un carro en la posición vertical mediante un controlador PD digital "
           "diseñado por lugar geométrico de las raíces en el plano z, e implementar en un ESP32 exactamente la ecuación "
           "en diferencias obtenida en MATLAB. El proyecto continúa el péndulo del cuatrimestre anterior, controlado con "
           "un PD analógico con amplificadores operacionales."))

h.append(H("2. Descripción del sistema"))
im = Image(IMAGEN, width=10.5 * cm, height=10.5 * cm * 473 / 535)
h.append(KeepTogether([im, Paragraph("Figura 1. Prototipo: riel V-Slot, carro con encoder y péndulo, motor NEMA 17 con banda GT2, "
                                     "driver DRV8825 y ESP32.", pie)]))
h.append(tabla([
    ["Elemento", "Especificación"],
    ["Microcontrolador", "ESP32 (NodeMCU-32S), doble núcleo a 240 MHz, framework Arduino en PlatformIO"],
    ["Actuador", "Motor a pasos NEMA 17 17HS4401, driver DRV8825 (≈1.3 A), micropasos 1/8"],
    ["Transmisión", "Banda GT2, polea de 20 dientes (40 mm/vuelta): 1600 pasos/vuelta = 40 000 pasos/m"],
    ["Sensor", "Encoder incremental LPD3806-600BM, 600 PPR × 4 = 2400 cuentas/vuelta (0.15°/cuenta)"],
    ["Parámetros", "g = 9.81 m/s², l = 0.342 m (pivote a centro de masa), recorrido útil del carro 0.88 m"],
], [3.4 * cm, 13.6 * cm]))

h.append(H("3. Modelo de la planta"))
h.append(P("El motor de CD del cuatrimestre anterior ya no estaba disponible y se sustituyó por un motor a pasos. "
           "Esto cambia la entrada de la planta: el motor de CD impone una <b>fuerza</b> y el movimiento del carro depende de su "
           "masa y de la reacción del péndulo; el motor a pasos impone directamente el <b>movimiento</b> del carro mientras no "
           "pierda pasos. Por eso la entrada pasa a ser la <b>aceleración del carro a</b> [m/s²]. De la ecuación linealizada del "
           "péndulo alrededor de la vertical:"))
h.append(E("l·θ'' = g·θ − a &nbsp;&nbsp;⇒&nbsp;&nbsp; G(s) = Θ(s)/A(s) = −1 / (l·s<super>2</super> − g)"))
h.append(P("Es el caso límite del modelo con fuerza cuando m/M → 0: el mismo sistema físico, con el mismo orden y el mismo "
           "polo inestable en s = +√(g/l) = +5.36 rad/s, por lo que la metodología de diseño de la clase se mantiene. "
           "El supuesto clave es que el motor no pierda pasos; por eso la aceleración comandada siempre se satura (sección 6)."))

h.append(H("4. Diseño del controlador digital (MATLAB)"))
h.append(P("<b>Paso 1. Tiempo de muestreo.</b> T = 0.01 s (100 Hz). La frecuencia de muestreo (628 rad/s) es unas 50 veces "
           "la frecuencia natural del lazo cerrado (12.3 rad/s) y unas 117 veces el polo inestable, de modo que el retenedor "
           "agrega poco atraso de fase."))
h.append(P("<b>Paso 2. Discretización con retenedor de orden cero:</b> G(z) = (1 − z<super>−1</super>)·Z{G(s)/s}"))
h.append(E("G(z) = 1.4623×10<super>−4</super>·(z + 1) / (z<super>2</super> − 2.002869·z + 1)"))
h.append(P("<b>Paso 3. Lugar geométrico de las raíces en z.</b> Se propuso un PD digital con un polo en el origen y un cero en zc:"))
h.append(E("D(z) = K·(z − zc) / z, &nbsp;&nbsp; K = 635.94, &nbsp; zc = 0.91875"))
h.append(P("El lazo cerrado es estable (todos los polos dentro del círculo unitario) para <b>121 &lt; K &lt; 6822</b>. "
           "Con el valor elegido, los polos de lazo cerrado quedan en z = 0.903 ± j0.065 (equivalentes a ωn ≈ 12.3 rad/s, "
           "ζ ≈ 0.81) y un polo rápido en z = 0.104. K queda lejos de ambos límites del margen."))
h.append(P("<b>Paso 4. Ecuación en diferencias.</b> Con e[k] = 0 − θ[k]:"))
h.append(E("D(z) = K·(1 − zc·z<super>−1</super>) &nbsp;⇒&nbsp; U(z) = K·E(z) − K·zc·z<super>−1</super>E(z)"))
h.append(E("<b>u[k] = 635.94·e[k] − 584.27·e[k−1]</b> &nbsp;&nbsp; (K·zc = 635.94 × 0.91875 = 584.27)"))
h.append(P("Como el signo negativo de la planta se absorbió en el diseño, la aceleración física del carro es a[k] = −u[k], "
           "es decir a[k] = 635.94·θ[k] − 584.27·θ[k−1]. La acción derivativa ya está contenida en el término e[k−1]; "
           "no se calcula una derivada por separado."))

h.append(H("5. Implementación en el ESP32"))
h.append(P("<b>Muestreo determinista.</b> El controlador corre en una tarea de FreeRTOS fijada al núcleo 0, con prioridad alta, "
           "que se repite cada 10 ms exactos con <i>vTaskDelayUntil</i>. En cada periodo: lee θ, calcula el error, aplica la "
           "ecuación en diferencias, satura, manda al motor y guarda e[k] como e[k−1]. La telemetría y los comandos por "
           "puerto serie corren en el núcleo 1 para no alterar el periodo, que se mide en cada ciclo."))
h.append(P("<b>Sensor.</b> El encoder se lee con el contador de pulsos de hardware (PCNT) con filtro activado, así no se pierden "
           "cuentas. El cero se toma con el péndulo colgando y quieto; la vertical es esa posición más 1200 cuentas (media vuelta), "
           "por lo que no depende de alinearlo a ojo."))
h.append(P("<b>Actuador.</b> El controlador entrega aceleración, pero el motor se maneja por velocidad. La conversión es explícita: "
           "v[k] = v[k−1] + a[k]·T, que se convierte a pasos/s (× 40 000). En cada periodo se pide al motor alcanzar v[k] con "
           "aceleración constante |a[k]|, de modo que la aceleración se mantiene durante T, igual que el retenedor de orden cero "
           "supuesto en el diseño. Los pulsos de paso los genera el hardware (librería FastAccelStepper)."))
h.append(P("<b>Condición inicial.</b> Al arrancar se usa e[k−1] = e[k] y v = 0. Con e[k−1] = 0, la primera muestra pediría "
           "≈111 m/s² con el péndulo a 10°; así pide los ≈9 m/s² esperados por el diseño."))
h.append(P("La Tabla 1 relaciona cada resultado del diseño con el código; la Figura 2 muestra la función del controlador."))
h.append(tabla([
    ["Resultado del diseño", "Dónde está en el código"],
    ["T = 0.01 s", "config.h: const float T = 0.01; &nbsp;main.cpp: vTaskDelayUntil cada 10 ticks de 1 ms"],
    ["K = 635.94 (coeficiente de e[k])", "config.h: COEF_ERROR_ACTUAL = 635.94"],
    ["K·zc = 584.27 (coeficiente de e[k−1])", "config.h: COEF_ERROR_ANTERIOR = 584.27"],
    ["e[k] = r − θ[k]", "control.cpp: errorActual = thetaReferencia − theta"],
    ["u[k] = 635.94 e[k] − 584.27 e[k−1]", "control.cpp: u = COEF_ERROR_ACTUAL·errorActual − COEF_ERROR_ANTERIOR·errorAnterior"],
    ["a = −u", "control.cpp: aceleracionComando = −u"],
    ["Retardo z<super>−1</super>", "control.cpp: errorAnterior = errorActual (al final de cada periodo)"],
], [5.6 * cm, 11.4 * cm]))
h.append(Paragraph("Tabla 1. Correspondencia entre el diseño en MATLAB y el programa del ESP32.", pie))
cod = """void controlCalcular(float theta, float posicionCarro) {
  // Referencia: 0 en el diseño original; el lazo externo (sección 7) la ajusta
  errorActual = thetaReferencia - theta;                               // e[k]
  // Ecuación en diferencias (paso 4): u[k] = 635.94 e[k] - 584.27 e[k-1]
  float u = COEF_ERROR_ACTUAL * errorActual - COEF_ERROR_ANTERIOR * errorAnterior;
  aceleracionComando = -u;                                             // a = -u
  aceleracionComando = constrain(aceleracionComando, -ACELERACION_MAXIMA, ACELERACION_MAXIMA);
  velocidadComando = velocidadComando + aceleracionComando * T;        // v[k] = v[k-1] + a[k]·T
  velocidadComando = constrain(velocidadComando, -VELOCIDAD_MAXIMA, VELOCIDAD_MAXIMA);
  errorAnterior = errorActual;                                         // e[k] -> e[k-1]
}"""
caja = Table([[Preformatted(cod, codigo)]], colWidths=[17 * cm])
caja.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), GRIS), ("BOX", (0, 0), (-1, -1), 0.4, colors.HexColor("#B8C2CF")),
                          ("LEFTPADDING", (0, 0), (-1, -1), 6), ("TOPPADDING", (0, 0), (-1, -1), 5), ("BOTTOMPADDING", (0, 0), (-1, -1), 5)]))
h.append(KeepTogether([caja, Paragraph("Figura 2. Controlador implementado (control.cpp, versión resumida).", pie)]))

h.append(H("6. Saturaciones y seguridad"))
h.append(P("En la prueba de banco el motor no perdió pasos hasta 7 m/s² (con 8 m/s² ya los perdía). Para conservar margen, la "
           "aceleración se satura en <b>5.5 m/s²</b> y la velocidad en <b>0.4 m/s</b>. Con la ecuación de diseño la saturación se "
           "alcanza a partir de unos 6° de inclinación; la simulación no lineal con estos límites es estable desde 2° hasta 10°. "
           "Protecciones: el control se apaga si |θ| &gt; 30° (el péndulo ya cayó) o si el carro se aleja más de 0.44 m del centro "
           "(límite por software con la cuenta de pasos), y solo arranca por comando con el carro centrado y |θ| &lt; 5°."))

h.append(H("7. Lazo externo de posición (cascada)"))
h.append(P("El controlador de la clase solo mide el ángulo, por lo que el péndulo se equilibra pero el carro deriva lentamente "
           "hasta el extremo del riel (en simulación, ≈0.7 m en 3 s). Para resolverlo se agregó un lazo externo lento que "
           "genera la referencia de ángulo del lazo interno; <b>el lazo interno es exactamente el diseño anterior, sin cambios</b>:"))
h.append(E("θref = −(0.05·x + 0.10·v), &nbsp; limitada a ±3°, &nbsp;&nbsp; e[k] = θref − θ[k]"))
h.append(P("Si el carro se aleja del centro, el lazo externo pide una pequeña inclinación hacia el centro; para sostenerla, el "
           "lazo interno acelera el carro de regreso. Las ganancias se eligieron como g·0.05 = ω² (ω ≈ 0.7 rad/s, unas 17 veces "
           "más lento que el lazo interno, para que no interfieran) y g·0.10 = 2ζω (ζ ≈ 0.7). En la primera prueba la referencia "
           "cambiaba en cada muestra y la parte derivativa del lazo interno la amplificaba hasta saturar el motor; se corrigió "
           "con un filtro de primer orden de la referencia (τ = 0.5 s)."))

h.append(H("8. Pruebas y resultados"))
h.append(P("Cada prueba se hizo en un programa separado que reutiliza los mismos módulos del programa final:"))
h.append(tabla([
    ["Prueba", "Criterio", "Resultado"],
    ["1. Encoder en banco", "1 vuelta = 2400 cuentas, 90° = 600, regresa a la marca", "2400, 600 y 0 cuentas al regresar: aprobada"],
    ["2. Motor en banco", "Aceleración máxima sin perder pasos", "7 m/s² a 0.5 m/s sin pérdidas; con 8 m/s² pierde pasos"],
    ["3. Controlador sin motor", "Periodo de 10 ms; aceleración coherente con la ecuación",
     "Periodo constante de 10 ms. Péndulo quieto a 3°: a = (635.94 − 584.27)·θ = 51.67·0.0524 = 2.71 m/s²; medido: 2.71 m/s²"],
    ["4. Verificación de signo", "El carro empuja hacia el lado al que cae el péndulo", "Correcto en ambos sentidos: aprobada"],
    ["5. Lazo cerrado (diseño de la clase)", "El péndulo se mantiene vertical", "Se equilibra sin vibración hasta que la deriva lleva el carro al límite del riel"],
    ["5b. Lazo cerrado + cascada", "Equilibrio sostenido", "Más de 20 s equilibrado, carro prácticamente al centro, θref suave"],
], [3.6 * cm, 5.2 * cm, 8.2 * cm]))
h.append(Paragraph("Tabla 2. Pruebas por fases.", pie))
h.append(P("La prueba 3 confirma que el ESP32 ejecuta exactamente la ecuación de diseño: el valor calculado a mano con los "
           "coeficientes de MATLAB coincide con el medido."))

h.append(H("9. Conclusiones"))
h.append(P("Se implementó en el ESP32 el controlador PD digital diseñado en MATLAB por lugar de las raíces en z, con la misma "
           "ecuación en diferencias, el mismo periodo de muestreo y un muestreo determinista verificado. El cambio a motor a pasos "
           "se justificó tomando la aceleración del carro como entrada, lo que conserva la estructura de la planta. El diseño "
           "estabilizó el péndulo en la vertical; la única limitación observada fue la deriva del carro, propia de controlar "
           "solo el ángulo, que se resolvió con un lazo externo de posición en cascada sin modificar el controlador de la clase."))
h.append(P("Como trabajo adicional se implementó un arranque desde la posición colgando (swing-up por energía), que no forma "
           "parte del diseño de la clase. Con la fricción medida en el eje del encoder, la simulación indica que el carro no puede "
           "aportar suficiente energía dentro de su recorrido y con las saturaciones de seguridad; queda como trabajo futuro "
           "reducir la fricción del pivote."))


def numerar(canvas, doc):
    canvas.saveState()
    canvas.setFont("DV", 8)
    canvas.setFillColor(colors.grey)
    canvas.drawRightString(letter[0] - 2 * cm, 1.2 * cm, f"U2EP2MTR09A_E8 · {doc.page}")
    canvas.restoreState()


doc = SimpleDocTemplate(SALIDA, pagesize=letter, leftMargin=2.2 * cm, rightMargin=2.2 * cm,
                        topMargin=1.8 * cm, bottomMargin=1.8 * cm,
                        title="Control digital de un péndulo invertido lineal", author=ALUMNO)
doc.build(h, onLaterPages=numerar)
