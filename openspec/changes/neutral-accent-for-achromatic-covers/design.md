## Context

El acento dinámico vive en `source/ui_theme.c`:

- `UI_CoverDominantColor` muestrea la carátula, ya decodificada como textura de vita2d, en una grilla de unos 48x48 puntos. Arma un histograma de 24 tonos, ponderado por saturación, con los píxeles de saturación ≥ 0.18 y luminosidad entre 0.10 y 0.92. Devuelve `SCE_FALSE` en tres casos que hoy no se distinguen: textura nula, formato no soportado, o menos de `UI_HUE_MIN_SHARE` (6 %) de píxeles con color.
- `UI_Theme_SetAccentFromCoverArt` (`ui_theme.c:935`) aplica `UI_MakeAccentLegible(dominante)` si hubo color, y si no, `UI_ACCENT_FIXED`.
- `UI_MakeAccentLegible` lleva la saturación a [0.60, 0.95] y la luminosidad a [0.58, 0.76], y después sube la luminosidad hasta tener contraste ≥ 4.5 sobre `UI_COLOR_BG`.

El acento se usa en tres formas:

1. **Como color de primer plano sobre el fondo:** ícono activo del nav rail, pestaña activa de Biblioteca, carpeta seleccionada, hint de categoría en Settings.
2. **Como velo** (`ui_color_accent_wash`, alfa 36): filas seleccionadas.
3. **Como relleno con algo encima:** el botón de play/pausa de Now Playing (`menu_audioplayer.c:300-304`) y el del mini player (`mini_player.c:69-73`), los dos con el símbolo en `UI_COLOR_TEXT_PRIMARY` (`#F4EFEA`). También el interruptor activo de Settings, pero ese ya dibuja la perilla en `UI_COLOR_BG`.

La forma 3 es la que el acento neutro rompe. Las cifras salen del cálculo de contraste hecho en la exploración:

| Acento | vs fondo | símbolo primario encima | vs texto secundario |
|---|---|---|---|
| naranja fijo `#FF9166` | 8.6 | 1.9 | 1.03 (se distingue por tono) |
| neutro medio `#B8B8B8` | 9.6 | 1.7 | 1.15 |
| neutro claro `#E6E3EC` | 15.0 | **1.1** | 1.80 |

Los datos de la biblioteca de prueba vienen de una réplica en Python del extractor, corrida sobre las 152 carátulas de la consola (ver proposal.md, Why).

## Goals / Non-Goals

**Goals:**
- Separar "sin carátula" de "carátula sin color" en una sola decisión, tomada en un solo lugar.
- Que el umbral nuevo no cambie ningún color que hoy ya se deriva bien.
- Poder probar en PC la clasificación y el umbral, sin la consola.

**Non-Goals:**
- Cambiar la forma de muestrear la carátula o de elegir el tono ganador.
- Revisar el contraste del símbolo blanco sobre acentos de color claros (por ejemplo, 1.7 sobre un azul claro). Es un problema que ya existe y queda igual. Ver Risks.

## Decisions

### 1. Extraer la lógica de color a un módulo puro

La conversión HSL, el histograma, la elección del pico, la clasificación y `UI_MakeAccentLegible` pasan de `ui_theme.c` a `source/accent.c` + `include/accent.h`. Ese módulo no incluye vita2d ni headers SCE: usa `unsigned int` para los colores RGBA8, `int` para los booleanos y recibe los píxeles ya leídos. En `ui_theme.c` queda la parte atada a vita2d: leer el formato y el stride de la textura, recorrer la grilla y entregar cada píxel al histograma.

- **Por qué:** el umbral y la clasificación son las afirmaciones centrales del cambio. Con el módulo puro se prueban en PC (`tests/test_accent.c`) con imágenes sintéticas, igual que `lang.c` y `screen_off.c`.
- **Alternativa descartada:** dejar todo en `ui_theme.c` y verificar solo en la consola. Es más barato, pero ninguna prueba repetible cuidaría el umbral ni la equivalencia con los colores actuales.
- Esta extracción va primero, en su propio commit y sin cambiar comportamiento.

### 2. Umbral `UI_HUE_MIN_SHARE` de 0.06 a 0.01

- **Por qué 1 %:** en la biblioteca de prueba separa limpio los dos grupos. Las portadas con un detalle de color quedan entre 3.5 % y 5.2 %. Las que son blanco y negro quedan en 0.5 % o menos: *Nothing Lasts Forever* 0.5 %, *The Demon* 0.4 %, *The Weekend* 0.2 %, el resto 0.1 % o 0.0 %.
- **Por qué no cambia ninguna portada que ya tiene color:** el umbral solo se compara contra el total en `UI_HueHistogramPeak`. No participa en qué píxeles votan ni en cuál tono gana. Una carátula que hoy supera el 6 % supera también el 1 % y produce el mismo pico (la réplica dio 0 de 136 cambios).
- **Alternativa descartada:** bajar también `UI_HUE_MIN_SAT` (0.18 a 0.10). Mueve el color de 85 de 136 portadas (en promedio 0.6° de tono, como máximo 8.9°) para recuperar dos tintes que el ajuste de legibilidad después exagera.

### 3. Clasificación de tres estados en lugar de un booleano

`UI_CoverDominantColor` se reemplaza por una función que devuelve uno de tres estados:

| Estado | Cuándo | Acento |
|---|---|---|
| `ACCENT_COVER_NONE` | textura nula, formato de textura no soportado, o puntero/tamaño inválido | `UI_ACCENT_FIXED` |
| `ACCENT_COVER_ACHROMATIC` | se pudo muestrear, pero hay menos del 1 % de píxeles con color, incluida la carátula JPEG en escala de grises (`U8_R`) | `UI_ACCENT_NEUTRAL` |
| `ACCENT_COVER_CHROMATIC` | hay un pico de tono | `MakeAccentLegible(pico)` |

- Un formato no soportado cuenta como "sin carátula" y no como "sin color": si no se pudo leer, no se sabe si tenía color. Es lo mismo que pasa hoy.
- **El acento neutro no pasa por `MakeAccentLegible`.** Esa función fuerza la saturación a 0.60 como mínimo, y con tono 0 un gris se volvería rojo. El neutro es una constante ya legible.

### 4. Valor del acento neutro: `UI_ACCENT_NEUTRAL = #E6E3EC`

- Es claro: contraste 15.0 sobre el fondo y 1.80 contra el texto secundario. Un neutro medio quedaría pegado al secundario (1.15) y no cumpliría el requirement de legibilidad.
- Tiene un tinte violeta mínimo, igual que todos los grises de la paleta (`TEXT_SECONDARY`, `TEXT_TERTIARY`, `SURFACE`). Un gris puro desentonaría con el resto. A la vista se lee como "sin color".
- No es igual a `UI_COLOR_TEXT_PRIMARY` (`#F4EFEA`, cálido): así un texto activo con acento neutro y un texto normal siguen siendo distintos, aunque por poco.
- El velo sale con la misma fórmula de siempre: el mismo color con alfa `UI_ACCENT_WASH_ALPHA`.
- El valor exacto se ajusta en la prueba en consola (ver Open Questions) sin tocar el spec.

### 5. Color para el contenido sobre el acento: `ui_color_on_accent`

Una tercera variable global, junto a `ui_color_accent` y `ui_color_accent_wash`, que se fija en el mismo `UI_Theme_ApplyAccent`:

- con `ACCENT_COVER_ACHROMATIC`: `UI_COLOR_BG`;
- en cualquier otro caso: `UI_COLOR_TEXT_PRIMARY`, igual que hoy.

Los cuatro llamados que dibujan el símbolo de play/pausa sobre el botón (`menu_audioplayer.c:302,304` y `mini_player.c:71,73`) pasan a usar `ui_color_on_accent`.

- **Por qué por estado y no por contraste:** elegir automáticamente el color de mayor contraste también daría un símbolo oscuro sobre el naranja fijo (8.6 contra 1.9) y sobre casi todos los acentos de color. Cambiaría el look de todas las portadas, que el spec dice que se conservan. Si eso se quiere, es otro cambio.
- El interruptor de Settings no se toca: su perilla ya usa `UI_COLOR_BG`.

## Risks / Trade-offs

- [Riesgo] El acento neutro claro se parece al texto primario. En el nav rail y en las pestañas, lo activo podría no destacar lo suficiente. → Mitigación: lo inactivo usa `TEXT_TERTIARY` y `TEXT_MUTED` (contraste 3.9 y mayor contra el neutro), así que activo e inactivo se separan bien. Igual se revisa en la consola pantalla por pantalla. Si no funciona, se aplica el criterio de descarte (Migration Plan).
- [Riesgo] Una carátula con ruido de color (JPEG muy comprimido) podría superar el 1 % y teñir la interfaz con un color al azar. → Mitigación: el píxel igual necesita saturación ≥ 0.18 y luminosidad en rango. En la biblioteca de prueba ninguna portada en blanco y negro pasó del 0.5 %. Se cubre con una prueba en PC de una imagen gris con ruido leve.
- [Trade-off] Detalles de color menores al 1 % (*The Demon*, *ARCHIVES*) quedan con el acento neutro. Es aceptable: el neutro representa bien una portada casi sin color.
- [Trade-off conocido, sin cambio] El símbolo blanco sobre acentos de color claros tiene contraste bajo (1.7 a 1.9). Ya pasa hoy y queda fuera del alcance.

## Migration Plan

Sin datos persistidos ni configuración que migrar. El acento se recalcula en cada carga de track.

**Criterio de descarte:** si el acento neutro no se ve bien en la consola, se revierten los commits del neutro y del color sobre el acento (Decisiones 3 a 5) y queda solo el umbral (Decisión 2) más el módulo puro (Decisión 1). En ese caso:
- `ACCENT_COVER_ACHROMATIC` vuelve a mapear a `UI_ACCENT_FIXED`;
- el delta del spec se ajusta a "solo umbral";
- las notas de versión pasan a patch (v3.2.1).

Las tareas están ordenadas para que ese corte quede limpio.

## Estrategia de pruebas

- **Compilación:** `.vpk` limpio con `-Wall -Werror` en cada commit.
- **Pruebas en PC (`make -C tests`)**, nuevo `test_accent`:
  - imagen gris con un 4 % de rojo saturado → `CHROMATIC` y tono rojo;
  - imagen gris con un 0.5 % de rojo → `ACHROMATIC`;
  - imagen blanca, negra y gris uniformes → `ACHROMATIC`;
  - gris con ruido leve de ±6 por canal → `ACHROMATIC`;
  - imagen con 20 % de color: el pico es igual con umbral 0.06 y con 0.01 (la equivalencia con los colores de hoy);
  - `MakeAccentLegible(#FF9166)` y dos colores más dan el mismo resultado que antes de extraer el módulo (valores fijados en la prueba antes de mover el código);
  - el color sobre el acento es `UI_COLOR_BG` con `ACHROMATIC` y `TEXT_PRIMARY` con los otros dos estados.
- **Consola**, con la biblioteca de la Vita: *Entropy* (rojo), *2* de i-dle (azul claro), *EASY* y *We are i-dle* (neutro), un MOD o WAV (naranja), *DAYDREAM* y *Shoot Me* (mismo color que en v3.2.0). Con *EASY*, se revisan Now Playing (botón de play pausado y en reproducción, barra de progreso), el mini player en Folders, el nav rail y el interruptor activo de Settings.

## CI

Sin cambios en el pipeline. `CMakeLists.txt` enumera las fuentes una por una en `add_executable`, así que `source/accent.c` se agrega a esa lista. El CI lo compila sin más cambios. Las pruebas en PC se siguen corriendo localmente, como `test_lang` y `test_screen_off`: el workflow de CI no las ejecuta hoy, y sumarlas queda fuera de este cambio.

## Open Questions

- El valor exacto de `UI_ACCENT_NEUTRAL` (`#E6E3EC` o algo cercano) se fija mirando la pantalla de la Vita. Cambiarlo no afecta el spec, las decisiones ni las tareas.
