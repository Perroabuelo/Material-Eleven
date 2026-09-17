## Context

Ver `proposal.md` - Why para la motivación. En términos técnicos, el estado actual es:

- `source/textures.c` carga como `vita2d_texture*` con filtro bilineal (`Texture_LoadImageBilinear`) todos los íconos de transporte, íconos de nav rail, toggle/radio y el atlas 9-slice de esquinas (`ui_corner_sm` 20x20 nativo/radio 10, `ui_corner_lg` 48x48 nativo/radio 24).
- `source/ui_theme.c:UI_DrawRoundedRect` compone un rect redondeado con 5 rectángulos planos + 4 quads de textura de esquina (`UI_DrawCornerAtlasQuad`), reescalando la textura nativa al radio pedido.
- `source/nav_rail.c:NavRail_DrawAndHitTest` dibuja cada ícono de 28px reescalado a 19px vía `vita2d_draw_texture_tint_scale`.
- `source/menus/menu_audioplayer.c` dibuja los botones de transporte centrando la textura nativa dentro de un contenedor de tamaño fijo (`SIDE_BTN_SIZE=44`), sin reescalar — de ahí que `btn_rewind`/`btn_forward` (68px nativos) desborden ese contenedor.
- `source/menus/menu_settings.c:SettingsUI_DrawRadio` YA dibuja el radio button con 3 `vita2d_draw_fill_circle` concéntricos, sin textura — es el único control ya vectorial hoy, y sirve de precedente probado en hardware.
- `include/ui_theme.h` define `UI_COLOR_ACCENT`/`UI_COLOR_ACCENT_WASH` como macros `#define` (constantes de compilación), usadas en 5 archivos / ~13 sitios.
- `metadata.cover_image` (`include/audio/audio.h`) es únicamente un `vita2d_texture*`, decodificado directo a GPU vía `vita2d_load_JPEG_buffer`/`vita2d_load_PNG_buffer` en `flac.c`/`mp3.c`/`opus.c`; no se retiene ningún buffer de píxeles en CPU aparte de la textura.
- `vita2d.h` es una dependencia externa del toolchain VITASDK, no vendorizada en este repo — no pudo verificarse contra el header real durante la exploración. Las decisiones de esta sección que dependen de su API están marcadas como a confirmar.

## Goals / Non-Goals

**Goals:**
- Eliminar el uso de texturas PNG para todo lo que sea un control de UI o chrome decorativo (íconos de transporte, nav rail, toggle, esquinas redondeadas), reemplazándolo por dibujo vectorial con las primitivas que vita2d expone.
- Introducir un color de acento en runtime derivado de la carátula del track actual, sustituyendo el acento fijo de compilación, aplicado uniformemente en las 3 pantallas y el nav rail.
- Mantener el árbol compilable y funcional en cada etapa de commits (ver `tasks.md`), no como un cambio monolítico.

**Non-Goals:**
- No se toca `default_artwork`/`default_artwork_blur` ni los íconos de batería (`battery_*`) — son contenido/indicadores reales, no controles de UI, y no forman parte de la queja que originó este change.
- No se implementa el algoritmo de extracción tonal real de Material You (HCT, quantizer de Celebi) — se simula con un método más simple, como el propio Reality Check del proyecto ya aceptó explícitamente.
- No se agrega ninguna nueva superficie de audio/metadata más allá de leer los píxeles ya decodificados de `cover_image`.

## Decisions

**Dibujo vectorial vía `vita2d_draw_array`, con apilado de rectángulos como plan B.** `vita2d` expone (según el código ya en uso y el conocimiento general de la librería, sin verificación directa del header en este entorno) una función de submisión genérica de vértices con un tipo de primitiva GXM (`TRIANGLES`/`TRIANGLE_STRIP`/`TRIANGLE_FAN`), que es el mismo mecanismo que la librería usa internamente para `draw_line` y `draw_fill_circle`. Se prefiere sobre apilar `vita2d_draw_rectangle` por fila de píxel (que funciona con las primitivas ya probadas hoy, pero cuesta ~1 draw call por fila de alto del ícono) y sobre bajar a SceGxm crudo (máximo control, pero primera vez que este código tocaría GXM por fuera de vita2d). **Primer task de implementación**: confirmar contra el header real de VITASDK instalado que esta función existe con esa firma; si no existe, cae a la ruta de apilado de rectángulos sin cambiar el resto del diseño.

**Mapeo de forma por ícono:**
| Ícono | Naturaleza | Técnica |
|---|---|---|
| Play | relleno, 1 triángulo | `TRIANGLES`, 3 vértices |
| Pause | relleno, 2 barras | `vita2d_draw_rectangle` x2 (sin cambios) |
| Forward / Rewind | relleno, 2 triángulos + 1 barra | `TRIANGLES` x2 + rect |
| Nav rail (now playing / folders / settings) | formas simples (círculo/rect/triángulo según mockup) | primitivas combinadas equivalentes |
| Shuffle / Repeat | trazo (`stroke`, no relleno), con `round cap` | 4-6 segmentos rectos como quads angostos (`TRIANGLE_STRIP`) + `vita2d_draw_fill_circle` chicos en las uniones para simular el cap redondeado |
| Radio button | ya vectorial hoy | sin cambios (`SettingsUI_DrawRadio`) |
| Toggle | relleno + pista | pista con `UI_DrawPill` (ya vectorial vía `UI_DrawRoundedRect`), perilla con `vita2d_draw_fill_circle` |

**Esquinas redondeadas vía arco vectorial (triangle fan), reemplazando el atlas 9-slice.** Un rect redondeado se compone de un rectángulo central (ya vectorial) más 4 arcos de 90°, cada uno un abanico de triángulos desde el centro de la esquina hasta N puntos sobre el arco (N chico, ej. 6-8, suficiente a los radios usados en la app: `UI_RADIUS_SM=10`, `UI_RADIUS_LG=24`). Esto retira `ui_corner_sm`/`ui_corner_lg` de `res/` y de `textures.c`/`textures.h`, y revierte la decisión de `add-ui-skin`'s `design.md` archivado ("Rounded corners via 9-sliced textures, not draw-time primitive composition") — la razón original (vita2d no tiene primitiva de rect redondeado) sigue siendo cierta, pero ya no es necesaria una textura si `draw_array` permite componer el arco a mano.

**Acento dinámico: lectura directa de la textura ya decodificada, no un segundo decode.** `metadata.cover_image` ya es un `vita2d_texture*` cuando hay carátula embebida. En vez de decodificar el JPEG/PNG una segunda vez solo para muestreo de color, se lee el buffer de la textura ya existente (Vita no tiene VRAM discreta; la memoria de textura es accesible desde CPU) submuestreando cada N-ésimo píxel, se arma un histograma simple por tono (hue) y se toma el bucket más frecuente como color dominante. **A verificar como parte de la implementación**: que el loader de `vita2d_load_JPEG_buffer`/`_PNG_buffer` cree la textura en layout lineal (no "swizzled"); si no es así, esta ruta lee datos incorrectos y hay que caer a un segundo decode dedicado a muestreo (opción B evaluada y descartada como primera opción solo por costo de CPU duplicado, no por incorrección).

**El color dominante se ajusta en HSL antes de usarse como acento**, acotando saturación y luminosidad a un rango legible sobre el fondo oscuro fijo (`UI_COLOR_BG`), en vez de usar el tono extraído crudo. Esto es lo que la especificación llama "se mantiene legible" y es, explícitamente, la parte "simulada" que tanto el Reality Check como este design aceptan como compromiso frente a un verdadero Material You (que en Android deriva de una paleta tonal HCT completa con múltiples stops de contraste garantizado).

**`UI_COLOR_ACCENT`/`UI_COLOR_ACCENT_WASH` pasan de macro a variable en runtime**, con un setter (ej. `UI_Theme_SetAccentFromCoverArt` / `UI_Theme_ResetAccent`) invocado en el mismo punto donde hoy se puebla `metadata.cover_image` (una vez por carga de track, no por frame) y en el punto donde se libera/no hay track (`Audio_HasTrack() == SCE_FALSE`), cayendo al valor fijo `#FF9166`. Los ~13 sitios que hoy usan la macro (`nav_rail.c`, `menu_settings.c`, `menu_audioplayer.c`, `menu_displayfiles.c`) pasan a leer la variable; no se distingue un "acento de sistema" separado del "acento de now playing" — por decisión explícita, todas las pantallas y el indicador de pantalla activa del nav rail comparten el mismo valor.

**Los commits se entregan por etapas**, no como un único commit — ver el desglose y el orden en `tasks.md`. Cada etapa deja el build funcional antes de la siguiente, de modo que un problema en una etapa posterior (por ejemplo, la lectura de textura para el acento dinámico) no bloquea haber mergeado ya la vectorización de íconos.

## Risks / Trade-offs

- [Riesgo] `vita2d_draw_array` (o el nombre real de la función equivalente) podría no existir con la firma asumida en el header de VITASDK instalado → Mitigación: primer task de implementación es verificarlo contra el header real; si no está, se cae a la ruta de apilado de rectángulos (más draw calls, mismo resultado visual) sin rediseñar nada más.
- [Riesgo] Los triángulos/arcos dibujados con primitivas GXM no tienen antialiasing garantizado (igual que las texturas que reemplazan, pero el patrón de aliasing puede ser perceptiblemente distinto — bordes diagonales "en escalera" en vez de un borde de textura ya suavizado en la fuente) → Mitigación: prototipar los íconos de transporte y el nav rail en hardware real antes de tocar las 3 pantallas, mismo enfoque de mitigación que ya usó `add-ui-skin` para las esquinas 9-slice.
- [Riesgo] La textura de `cover_image` podría no estar en layout lineal, invalidando la lectura directa de píxeles → Mitigación: verificar el layout antes de implementar la extracción; si es "swizzled", decodificar una segunda vez solo para muestreo (más costo de CPU, una vez por track, aceptable).
- [Riesgo] Un color extraído de una carátula con paleta extrema (muy oscura, muy clara, o monocromática) podría resultar ilegible pese al ajuste HSL → Mitigación: los rangos de saturación/luminosidad del ajuste se acotan explícitamente contra el fondo oscuro fijo conocido de la app (no contra un fondo arbitrario), y se prueban con carátulas de casos extremos (portada negra, portada blanca, portada en escala de grises) antes de dar por cerrada esa etapa.
- [Riesgo] Retirar `ui_corner_sm`/`ui_corner_lg`/íconos de `res/` y de `textures.c` sin dejar referencias colgantes → Mitigación: la etapa que los retira es la última de la secuencia de commits, después de confirmar que ningún call site vectorial nuevo depende todavía de la textura vieja.

## Open Questions

Ninguna — las decisiones de enfoque (vectorial vs. textura para íconos y esquinas, alcance global del acento dinámico, fallback sin carátula) ya quedaron resueltas en la conversación de exploración previa a este proposal.
