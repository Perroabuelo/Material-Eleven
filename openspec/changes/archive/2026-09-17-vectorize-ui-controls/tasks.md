## 1. Verificación técnica previa (bloquea todo lo demás, sin commit propio)

- [x] 1.1 Confirmar en el header real de VITASDK instalado (`vita2d.h`) si existe una función de dibujo por arreglo de vértices con primitiva GXM configurable (la asumida en `design.md` como `vita2d_draw_array`); verificar consultando el header instalado por el toolchain, no solo el uso ya existente en este repo. Dejar registrada la firma exacta encontrada (o su ausencia) antes de continuar.
  - **Confirmado.** Toolchain en WSL (Ubuntu-24.04): `/home/perroabuelo/vitasdk`. En `arm-vita-eabi/include/vita2d.h:100`:
    `void vita2d_draw_array(SceGxmPrimitiveType mode, const vita2d_color_vertex *vertices, size_t count);`
    con `typedef struct vita2d_color_vertex { float x; float y; float z; unsigned int color; }` (`vita2d.h:21-26`). La firma coincide con la asumida en `design.md`.
- [x] 1.2 Si la función del punto 1.1 no existe con la firma asumida, confirmar la ruta alternativa (apilado de `vita2d_draw_rectangle` por fila de píxel) como la que se va a implementar en las secciones 2 y 3, y ajustar sus tareas en consecuencia antes de escribir código.
  - **No aplica.** La función de 1.1 existe con la firma asumida, así que no se activa la ruta alternativa; las secciones 2 y 3 se implementan con `vita2d_draw_array` sin ajustes.
- [x] 1.3 Confirmar si `vita2d_load_JPEG_buffer`/`vita2d_load_PNG_buffer` crean la textura resultante en layout lineal (legible directamente desde CPU) o "swizzled"; dejar registrada la respuesta antes de empezar la sección 5.
  - **Confirmado: layout lineal.** `nm libvita2d.a` muestra `sceGxmTextureInitLinear` como el único inicializador de textura referenciado por la librería (no hay referencia a `sceGxmTextureInitSwizzled`), por lo que toda textura creada por vita2d — incluidas las de los loaders JPEG/PNG — es lineal. La lectura desde CPU se hace con `vita2d_texture_get_datap` (`vita2d.h:114`), `vita2d_texture_get_stride` (`vita2d.h:112`) y `vita2d_texture_get_format` (`vita2d.h:113`). No se necesita el segundo decode de respaldo.

## 2. Vectorizar íconos de transporte y nav rail (commit propio)

> **Nota de verificación (aplica a toda esta sección y a 3.1/3.2, 5.2, 6.3):** en este
> entorno no hay Vita ni emulador disponible. Cada tarea se da por completa con
> implementación + compilación limpia (`-Wall -Werror`, toolchain VITASDK en WSL) +
> validación de geometría replicando las mismas fórmulas en un render offline.
> La verificación visual en hardware real queda consolidada en 7.1, que sigue abierta.

- [x] 2.1 Implementar el dibujo vectorial de Play/Pause (triángulo relleno + 2 barras) en `menu_audioplayer.c`, reemplazando el uso de `btn_play`/`btn_pause`, y verificar visualmente en hardware o emulador que el ícono se ve completo y nítido dentro de su botón, sin desborde.
  - Implementado como `UI_DrawPlayGlyph`/`UI_DrawPauseGlyph` en `ui_theme.c` (compartidos con el mini-player, ver 2.7). Glifo de 30px dentro del botón de 76px.
- [x] 2.2 Implementar el dibujo vectorial de Forward/Rewind (2 triángulos + barra) en `menu_audioplayer.c`, reemplazando `btn_forward`/`btn_rewind`, y verificar que el ícono queda contenido dentro de `SIDE_BTN_SIZE` (44px), corrigiendo el desborde actual.
  - `UI_DrawSkipGlyph` en `ui_theme.c`. Con `SKIP_GLYPH_SIZE = 28` el glifo mide 19.9 x 19.0 px, contenido dentro de los 44px del botón (antes: textura de 68px nativos que desbordaba).
- [x] 2.3 Implementar el dibujo vectorial de Shuffle/Repeat (segmentos + `vita2d_draw_fill_circle` en las uniones) en `menu_audioplayer.c`, reemplazando `btn_shuffle`/`btn_repeat` y sus variantes `_overlay`, y verificar que el estado activo/inactivo de cada uno sigue siendo distinguible.
  - `Menu_DrawShuffleGlyph`/`Menu_DrawRepeatGlyph` (estáticas en `menu_audioplayer.c`), construidas con `UI_DrawStroke` (quad + dos `vita2d_draw_fill_circle` como round caps) más `UI_DrawTriangle` para las puntas de flecha. El par de estados reemplaza al par de texturas: activo `UI_COLOR_TEXT_PRIMARY` / inactivo `UI_COLOR_TEXT_TERTIARY`, equivalente al blanco (`FFFFFF`) vs. gris (`7F8082`) que tenían los PNG `_overlay` y base.
- [x] 2.4 Implementar el dibujo vectorial de los 3 íconos del nav rail (Now Playing, Folders, Settings) en `nav_rail.c`, reemplazando `icon_nav_*`, y verificar que cada ícono se distingue con claridad a su tamaño real en pantalla (sin el reescalado 28px→19px actual).
  - Triángulo relleno (Now Playing), contorno de carpeta con pestaña vía `UI_DrawStroke` (Folders) y engranaje vía `UI_DrawRing` + 8 dientes `UI_DrawQuad` (Settings). Dibujados a 22px reales; ya no hay reescalado. El engranaje usa un anillo real (no un círculo tapado con el color de fondo), de modo que también se ve correcto sobre el wash de acento del botón activo.
- [x] 2.5 Implementar el toggle de Ajustes (pista ya vectorial vía `UI_DrawPill` + perilla con `vita2d_draw_fill_circle`) en `menu_settings.c`, reemplazando `toggle_on`/`toggle_off`, y verificar que el estado encendido/apagado se distingue igual que antes.
  - `SettingsUI_DrawToggle`: pista 48x26 con `UI_DrawPill`, perilla r=10 con `vita2d_draw_fill_circle`. Encendido: pista acento + perilla `UI_COLOR_BG` a la derecha; apagado: pista `UI_COLOR_SURFACE_2` + perilla `UI_COLOR_TEXT_TERTIARY` a la izquierda.
  - **Hallazgo:** `toggle_on.png` y `toggle_off.png` eran ambos negro puro (`000000`) y se dibujaban sin tinte, así que sobre `UI_COLOR_BG` el control era prácticamente invisible. La versión vectorial es por tanto más legible que la anterior, no solo "igual de distinguible".
- [x] 2.6 Confirmar que las 3 pantallas (Now Playing, Folders, Settings) compilan y se ven correctamente con estos íconos vectoriales, y crear un commit que agrupe únicamente este trabajo.
  - Compila sin warnings con `-Wall -Werror` (`arm-vita-eabi-gcc` 15.2.0, VITASDK en WSL).
- [x] 2.7 **(añadida durante la implementación)** Vectorizar también los controles de transporte del mini-player de Folders (`menu_displayfiles.c`), que usa `btn_rewind`/`btn_play`/`btn_pause`/`btn_forward`.
  - No estaba listada: el mini-player se agregó en `add-ui-skin` (bloque 4) después de redactarse el alcance de este change. Sin vectorizarlo, la tarea 4.2 no puede retirar esas cuatro texturas. Resuelto promoviendo los glifos play/pause/skip a `ui_theme.c` para que ambas pantallas los compartan.

## 3. Vectorizar esquinas redondeadas (commit propio)

- [x] 3.1 Implementar el arco vectorial (triangle fan) para una esquina de radio `UI_RADIUS_SM` y otra de `UI_RADIUS_LG` como funciones auxiliares, y verificar visualmente que el radio resultante coincide con el de la textura que reemplazan.
  - Implementado como **una sola** auxiliar parametrizada por radio (`UI_DrawCornerArc` en `ui_theme.c`), no dos horneadas: los call sites usan radios 2, 8, 9, 11, 12, 16, 19, 26 y 38, no solo `UI_RADIUS_SM`/`UI_RADIUS_LG`; la ruta anterior los cubría reescalando el atlas, la vectorial los dibuja exactos.
  - **Coincidencia de radio verificada numéricamente:** midiendo el borde alfa de `ui_corner_sm` (20x20) y `ui_corner_lg` (48x48) contra un cuarto de círculo ideal de radio 10 y 24, la desviación máxima es 0.38 px y 0.49 px — es decir, los atlas *son* cuartos de círculo exactos y el arco del mismo radio reproduce su forma.
  - Error de inscripción del abanico con `segments = clamp(radio * 0.7, 6, 16)`: 0.063 px a r=10, 0.029 px a r=24, 0.046 px a r=38, 0.017 px a r=2. Por debajo del píxel en todo el rango usado.
- [x] 3.2 Reescribir `UI_DrawRoundedRect` en `ui_theme.c` para componerse enteramente de rectángulo central + 4 arcos vectoriales, eliminando la dependencia de `ui_corner_sm`/`ui_corner_lg`, y verificar que cards, chips, botones y el highlight de fila activa en las 3 pantallas se siguen viendo con esquinas redondeadas consistentes.
  - Misma descomposición que antes (banda central + 2 franjas laterales + 4 esquinas), con los 4 quads de textura reemplazados por 4 arcos. Se conservó deliberadamente la propiedad de no-solapamiento entre bandas y esquinas: `UI_DrawRoundedRect` también se llama con colores semitransparentes (`UI_COLOR_ACCENT_WASH` alfa 36 en el highlight de fila activa, `UI_COLOR_HAIRLINE` alfa 15), donde un solapamiento mostraría una costura por doble blending.
  - Los 11 call sites (`dirbrowse.c`, `menu_audioplayer.c`, `menu_displayfiles.c`, `menu_settings.c`, `nav_rail.c`, más `UI_DrawPill`/`UI_DrawRowHighlight`) pasan todos por esta función, así que el cambio queda contenido en ella. Confirmado con `get_context(callers)` de RepoWise.
  - `ui_theme.c` ya no incluye `textures.h`.
- [x] 3.3 Crear un commit que agrupe únicamente este trabajo.

## 4. Retirar texturas de control que quedan sin uso (commit propio)

- [x] 4.1 Confirmar (por ejemplo, con `get_dead_code` o una búsqueda de referencias) que ningún call site sigue usando `btn_play`, `btn_pause`, `btn_forward`, `btn_rewind`, `btn_repeat`, `btn_repeat_overlay`, `btn_shuffle`, `btn_shuffle_overlay`, `icon_nav_now_playing`, `icon_nav_folders`, `icon_nav_settings`, `toggle_on`, `toggle_off`, `ui_corner_sm`, `ui_corner_lg` tras las secciones 2 y 3.
  - Búsqueda de referencias sobre todos los `*.c`/`*.h` del repo: los 15 símbolos aparecen **únicamente** en `textures.c` (declaración `extern`, carga, liberación) y `textures.h` (declaración). Ningún call site los usa.
- [x] 4.2 Eliminar esas variables, sus cargas (`Textures_Load`) y liberaciones (`Textures_Free`) de `textures.c`/`textures.h`, y sus archivos `.png` correspondientes de `res/`, dejando intactos `icon_dir`, `icon_file`, `icon_audio`, `icon_back`, `radio_on`/`radio_off` (aún sin decisión de reemplazo en este change salvo lo indicado en 2.5) y los recursos de batería/carátula.
  - 15 PNG eliminados de `res/`: los 8 `btn_playback_*`, los 3 `icon_nav_*`, `toggle_on`/`toggle_off`, `ui_corner_sm`/`ui_corner_lg`.
  - **Observación:** `icon_back`, `radio_on` y `radio_off` tampoco tienen call site hoy (el radio button es vectorial desde `add-ui-skin`), pero esta tarea indica explícitamente conservarlos, así que quedan declarados y cargados. Candidatos para un change futuro.
- [x] 4.3 Verificar que el proyecto compila sin referencias colgantes a los símbolos `_binary_res_*` retirados, y crear un commit que agrupe únicamente esta limpieza.
  - Reconfiguración y build limpios desde cero (el glob `res/*.png` de CMake se resuelve en configure). `arm-vita-eabi-nm` sobre el ejecutable: cero coincidencias de `_binary_res_(btn_playback|icon_nav|toggle_|ui_corner)`, y los únicos símbolos indefinidos son los débiles estándar de newlib/gcc (`__libc_fini`, `_ITM_*`, etc.).

## 5. Acento dinámico: extracción de color desde la carátula (commit propio)

- [x] 5.1 Implementar la lectura del buffer de píxeles de `metadata.cover_image` (vía el mecanismo confirmado en 1.3) con submuestreo, y una función que arme un histograma simple por tono y devuelva el color dominante.
  - `UI_CoverDominantColor` en `ui_theme.c`: rejilla de ~48x48 muestras (una vez por carga de track), histograma de 24 buckets de tono, voto ponderado por saturación, promedio RGB del bucket ganador.
  - **Hallazgo importante sobre el formato de píxel:** el desensamblado de `libvita2d.a` muestra que los dos loaders producen formatos distintos — `vita2d_load_PNG_buffer` usa `vita2d_create_empty_texture` (4 B/px, `U8U8U8U8_ABGR`), mientras que `vita2d_load_JPEG_buffer` pide `U8U8U8_BGR` (3 B/px) o, para un JPEG en escala de grises, `U8_R` (1 B/px). Como la mayoría de las carátulas embebidas son JPEG, una implementación que asumiera RGBA8 habría leído basura. El código consulta `vita2d_texture_get_format()` y maneja los tres casos, devolviendo `SCE_FALSE` ante cualquier otro.
  - `vita2d_texture_get_stride()` para el avance de fila (GXM alinea el ancho), y descarte de píxeles con alfa < 128 en el formato de 4 bytes.
- [x] 5.2 Implementar el ajuste en HSL del color dominante a un rango de saturación/luminosidad legible sobre `UI_COLOR_BG`, y verificar con al menos tres carátulas de prueba (una muy oscura, una muy clara, una en escala de grises) que el resultado sigue siendo distinguible del fondo y del texto secundario, cubriendo los escenarios de `specs/ui/dynamic-accent/spec.md`.
  - `UI_MakeAccentLegible`: acota S a [0.60, 0.95] y L a [0.58, 0.76], la banda que rodea al acento fijo `#FF9166` (S 1.00 / L 0.70).
  - **Corrección surgida de la verificación:** acotar solo en HSL no alcanza. La luminosidad HSL no representa la luminancia percibida (el azul aporta 0.0722 frente a 0.7152 del verde), así que un violeta vivo a L 0.60 quedaba en **4.02:1** contra el fondo. Se añadió una segunda pasada que sube L en pasos de 0.02 hasta superar un piso de contraste WCAG real de 4.5:1 (techo L 0.88). El violeta pasa a L 0.64 y **4.81:1**.
  - **Verificado ejecutando el `ui_theme.c` real**: se compiló el archivo fuente del repo para el host (gcc, con stubs mínimos de `vita2d.h`/`psp2/types.h`) y se le pasaron carátulas sintéticas en los tres formatos de píxel. Arnés en `scratchpad/accent_test.c` + `run_accent_test.sh` (no se agrega al repo: el proyecto no tiene infraestructura de tests y el proposal no la contempla).

    | Caso | Formato | Dominante | Acento | vs fondo | vs texto sec. |
    |---|---|---|---|---|---|
    | Muy oscura (rojo apagado) | JPEG 3 B/px | `#2E0C0E` | `#D4545B` | 4.72:1 | 1.76:1 |
    | Muy clara (celeste pálido) | PNG 4 B/px | `#CDE2F8` | `#94C1F0` | 10.07:1 | 1.21:1 |
    | Escala de grises | JPEG gris 1 B/px | (sin tono usable) | `#FF9166` fijo | 8.58:1 | 1.03:1 |
    | Normal (verde azulado) | JPEG 3 B/px | `#16A89A` | `#42E6D6` | 12.22:1 | 1.47:1 |
    | Violeta (tono más cercano al texto sec.) | PNG 4 B/px | `#8460D2` | `#8F6CDA` | 4.81:1 | 1.73:1 |

  - Referencia: el acento fijo actual da 8.58:1 contra el fondo y 1.03:1 contra el texto secundario — es decir, hoy ya se distingue del texto secundario por saturación (S 1.00 vs 0.20), no por luminancia, y el piso `UI_ACCENT_MIN_SAT = 0.60` preserva esa separación.
  - Escenario "escala de grises": el histograma no encuentra suficientes píxeles cromáticos (umbral 6% de las muestras) y devuelve `SCE_FALSE`, de modo que el acento queda en el fijo — nunca indistinguible del gris.
- [x] 5.3 Crear un commit que agrupe únicamente esta extracción de color (sin todavía conectarla a la interfaz).

## 6. Acento dinámico: reemplazar el acento fijo en toda la interfaz (commit propio)

- [x] 6.1 Convertir `UI_COLOR_ACCENT`/`UI_COLOR_ACCENT_WASH` de macro a variable en runtime en `ui_theme.c`/`ui_theme.h`, con un setter que aplica el color derivado y otro que restaura el valor fijo `#FF9166`.
  - Las dos macros se retiran. En su lugar: `extern unsigned int ui_color_accent` / `ui_color_accent_wash` (misma convención de nombres que los `font_ui`/`font_mono` ya existentes en ese header), más `UI_ACCENT_FIXED` y `UI_ACCENT_WASH_ALPHA` como constantes.
  - `UI_Theme_SetAccentFromCoverArt(const vita2d_texture *cover)` y `UI_Theme_ResetAccent(void)`. El wash se recalcula a partir del acento con alfa 36, de modo que los dos nunca quedan desincronizados.
- [x] 6.2 Invocar el setter de la sección 5 cuando se puebla `metadata.cover_image` para un track nuevo (en los 3 decodificadores: `flac.c`, `mp3.c`, `opus.c`) y el de restauración cuando el track no tiene carátula o cuando `Audio_HasTrack()` es falso, cubriendo los escenarios de fallback de `specs/ui/dynamic-accent/spec.md`.
  - **Desviación de la ubicación indicada (mismo comportamiento, un solo punto):** en vez de 7 llamadas repartidas por los sitios donde `flac.c`/`mp3.c`/`opus.c` asignan `cover_image`, se invoca **una vez** en `Audio_Init`, inmediatamente después de `(*decoder.init)(path)` — que es justamente donde esos tres decodificadores acaban de decodificar la carátula. Motivo: la tarea también pide restaurar el acento "cuando el track no tiene carátula", y eso los decodificadores no pueden hacerlo (WAV, OGG y los trackers nunca tocan `cover_image`, y las ramas sin carátula de FLAC/MP3/OPUS tampoco). El punto único cubre ambos casos: `UI_Theme_SetAccentFromCoverArt(cover ? cover : NULL)` deriva cuando hay carátula y restaura cuando no. El guard usado (`metadata.has_meta && metadata.cover_image`) es exactamente el mismo con el que las pantallas deciden dibujar la carátula, así que acento y carátula nunca discrepan.
  - Restauración por `Audio_HasTrack() == SCE_FALSE`: `UI_Theme_ResetAccent()` en `Audio_Term`, junto al `track_loaded = SCE_FALSE` y al limpiado de `metadata`.
  - Arranque sin track: las variables se inicializan estáticamente en `UI_ACCENT_FIXED`.
  - Trazado de los 5 escenarios de `specs/ui/dynamic-accent/spec.md`:

    | Escenario | Ruta | Resultado |
    |---|---|---|
    | Track con carátula empieza a sonar | `Menu_PlayAudio` → `Menu_InitMusic` → `Audio_Init` → `decoder.init` puebla `cover_image` → setter | acento derivado |
    | Acento se mantiene al cambiar de pantalla | variable global, solo se escribe al cargar/terminar track | mismo acento |
    | Cambia el track | `Music_HandleNext` → `Audio_Term` (reset) → `Audio_Init` (set) | acento del nuevo track |
    | Track sin carátula (WAV, MOD/IT/S3M) | `cover_image` NULL → setter con NULL | `#FF9166` fijo |
    | Ningún track cargado | inicialización estática + `Audio_Term` | `#FF9166` fijo |

- [x] 6.3 Actualizar los ~13 sitios que hoy leen la macro (`nav_rail.c`, `menu_settings.c`, `menu_audioplayer.c`, `menu_displayfiles.c`) para leer la variable en runtime, y verificar en hardware o emulador que el nav rail (incluido su indicador de pantalla activa), Now Playing, Folders y Settings muestran el mismo acento derivado mientras un track con carátula está en reproducción, y que todos vuelven al acento fijo al reproducir un track sin carátula.
  - 12 sitios actualizados: `nav_rail.c` 2 (incluido el wash del indicador de pantalla activa), `menu_settings.c` 6, `menu_audioplayer.c` 2, `menu_displayfiles.c` 1, `ui_theme.c` 1 (`UI_DrawRowHighlight`). Una búsqueda de `UI_COLOR_ACCENT` sobre el repo ya no devuelve nada: todos leen la variable, así que las cuatro pantallas comparten el mismo valor por construcción.
  - Verificación visual en hardware: pendiente, ver la nota de la sección 2 y la tarea 7.1.
- [x] 6.4 Crear un commit que agrupe únicamente esta conexión final del acento dinámico a la interfaz.

## Commits

`openspec/` está en `.gitignore`, así que los commits contienen solo código y recursos.

| Commit | Sección |
|---|---|
| `c3db26a` Draw transport, nav rail and toggle controls as vectors | 2 |
| `d8740f4` Compose rounded corners from vector arcs, not the 9-slice atlas | 3 |
| `bec379f` Drop the control PNGs the vector drawing replaced | 4 |
| `c37e3c6` Derive a legible accent color from cover art | 5 |
| `60b8ca2` Make the interface accent follow the current cover art | 6 |
| `e608d81` Split cover sampling into smaller functions | (limpieza posterior, ver abajo) |
| `ac8f53d` Fix GPU crash: vector vertices must come from the frame pool | (corrección de 2/3, ver abajo) |
| `f3d908e` Retire the in-flight frame before tearing a track down | (corrección de 8.1) |

### Fallo encontrado en hardware (primera instalación)

La app crasheaba al abrir con *"An error has occurred. The PS Vita system will be powered off"*. Los volcados en `ux0:/data/` eran `psp2core-*-GPUCRASH.psp2dmp`, no excepciones de CPU, lo que apuntaba a GXM.

**Causa:** los cuatro helpers vectoriales (`UI_DrawTriangle`, `UI_DrawQuad`, `UI_DrawRing`, `UI_DrawCornerArc`) armaban los vértices en un arreglo local y se lo pasaban a `vita2d_draw_array`. El desensamblado de esa función muestra que **no copia**: pasa el puntero directo a `sceGxmSetVertexStream` y `sceGxmDraw` solo encola el dibujo. La GPU lee esos vértices después, al vaciar el frame — cuando el marco de pila ya no existe, y además la pila no está mapeada en el espacio de direcciones de la GPU.

**Corrección:** reservar con `vita2d_pool_memalign` (`vita2d.h:92`), que es memoria visible por GPU, vive hasta el flush del frame y la resetea `vita2d_start_drawing`. Es exactamente lo que hace el propio `vita2d_draw_fill_circle` (`vita2d_pool_memalign(1616, 16)`). El pool son 1 MB por frame contra decenas de KB de vértices aquí, y un `NULL` ahora omite la figura en vez de escribir a través de él.

**Por qué no lo detectó la verificación previa:** el arnés de host stubbea `vita2d_draw_array` como no-op, así que validó la aritmética de color pero nunca la semántica de GPU; y el render de geometría comprobaba formas, no vida útil de memoria. Ninguna de las dos técnicas podía encontrar esto — solo el hardware. Queda como límite conocido del enfoque sin dispositivo.

Datos verificados de paso: el buffer de índices lineales de vita2d son 65536 entradas (`gpu_alloc` de 0x1FFFE bytes), muy por encima de los ≤50 que pide el anillo, así que nunca fue un factor.

El último commit no corresponde a una tarea: el self-check de salud con RepoWise mostró que `UI_CoverDominantColor` había quedado con complejidad ciclomática 26, bajando `ui_theme.c` de 7.95 a 5.58. Se descompuso en setup del muestreador, lectura de píxel e histograma. Los cinco casos del arnés devuelven acentos idénticos byte a byte.

## 8. Pendientes tras la primera prueba en hardware (2026-09-16)

> **Cierre (2026-09-17):** 8.1 era la única regresión de este change y quedó resuelta en `f3d908e`.
> 8.2 y 8.3 se sacan del alcance por decisión del usuario: ninguna es regresión de esta vectorización
> (8.2 viene de `add-ui-skin`; 8.3 es el riesgo de aliasing que `design.md` ya aceptaba), y se retoman
> en un change aparte. Se conservan aquí, con sus puntos de partida, para que ese change pueda arrancar
> desde esta investigación en vez de repetirla.

La app **ya arranca y se ve** con `ac8f53d`. Quedan tres cosas abiertas, diferidas a otra sesión por decisión del usuario.

- [x] 8.1 **[Regresión — RESUELTA en `f3d908e`] Crash de GPU al cambiar de canción varias veces.**
  - Evidencia: `ux0:/data/psp2core-1789610094-GPUCRASH.psp2dmp`, 2026-09-16 22:54:56, **322 KB**. Sigue siendo `GPUCRASH` (no excepción de CPU), igual que los dos del arranque (22:44:08 y 22:45:45, 94 y 98 KB) — pero mucho más grande, lo que probablemente indique un estado de GPU distinto al del fallo ya corregido.
  - No hay volcados `GPUCRASH` anteriores a las 22:44 de hoy en la tarjeta, así que es una regresión de este change, no algo preexistente. El `.vpk` anterior está en `ux0:/ElevenMPV-prev-add-ui-skin.vpk` si se necesita comparar.
  - **No diagnosticado.** No asumir que es la misma causa que `ac8f53d`: eso ya está corregido y verificado. Sitios a mirar primero, por orden de sospecha:
    1. `UI_CoverDominantColor` leyendo la textura de carátula en `Audio_Init` — es lo único nuevo en la ruta de cambio de pista, y toca memoria de GPU desde CPU.
    2. `Music_FreeCurrentTrack` (`menu_audioplayer.c`) libera `metadata.cover_image` y **no lo pone a NULL**; lo limpia después `Audio_Term` vía `metadata = empty_metadata`. Verificar que no queda ninguna ventana en la que se dibuje o muestree la textura ya liberada.
    3. Agotamiento del pool de frame: `UI_VertexBuffer` devuelve `NULL` y omite la figura, lo que debería degradar en vez de crashear — confirmar que efectivamente no crashea por esa vía.
  - Herramienta sugerida: `vita-parse-core` sobre el `.psp2dmp` junto al `ElevenMPV.elf`/`.velf` del build, para ubicar el fallo.

  **Análisis forense del volcado (2026-09-17).** Los `.psp2dmp` son ELF core de ARM comprimidos con gzip, legibles con `readelf`/`objdump` del propio toolchain. Resultados:

  | Hipótesis | Veredicto | Evidencia |
  |---|---|---|
  | Primitiva `TRIANGLE_FAN` no soportada | **Descartada** | `vita2d_draw_fill_circle` la usa ella misma (`r1=0x10000000`), y `draw_rectangle` usa `TRIANGLE_STRIP` (`0x0c000000`). Ambas probadas en este hardware. |
  | Desbordar el pool de frame corrompe memoria | **Descartada** | `vita2d_pool_memalign` sí comprueba límite (`cmp r0,r3` / `movcs r0,#0`): devuelve NULL. El chequeo de NULL que ya tiene `UI_VertexBuffer` degrada la figura, no corrompe. |
  | Exceder el buffer de índices lineales | **Descartada** | 65536 entradas (`gpu_alloc` de 0x1FFFE B). El máximo que pido son 50. |
  | Fuga de texturas de carátula por cambio de pista | **Descartada** | `MEM_BLK_INFO`: 39 bloques `gpu_mem`/22.80 MB en el arranque vs 40/24.05 MB al crashear. El extra es **un** bloque de 1280K @ 0x62f00000 — una sola carátula viva, el estado esperado. |
  | Bug en `UI_CoverDominantColor` (lectura de la textura) | **Descartada por tipo de fallo** | Leer fuera de rango o un puntero colgante desde CPU daría un *abort de CPU*; los tres volcados son `GPUCRASH`. |

  **Conclusión:** el fallo está en la ruta de **dibujo**, no en la extracción de acento. Todos los hilos aparecen bloqueados dentro de SceGxm (`PC=0xe0006d54`), sin ninguno en código de la app, lo que confirma un fallo asíncrono de GPU. No se pudo extraer la dirección de fallo: `GPU_INFO` (13316 B) solo difiere en direcciones de buffers y no hay salida en `TTY_INFO`.

  **Hipótesis viva:** el cambio de pista introduce una discontinuidad de temporización grande (`sceKernelDelayThread(100ms)` en `Audio_Term` más el decode del JPEG) mientras hay un frame en vuelo, y el `vita2d_pool_reset()` del siguiente `vita2d_start_drawing` puede pisar vértices que la GPU aún está leyendo. Mi cambio multiplicó el tráfico del pool (decenas de reservas por frame frente a unas pocas antes), lo que haría que una carrera latente se manifieste. Nótese que el `vita2d_wait_rendering_done()` existente en `Music_FreeCurrentTrack` **solo se ejecuta si la pista tenía carátula**.

  **Hipótesis CONFIRMADA en hardware (2026-09-17).** La build de diagnóstico con un `vita2d_wait_rendering_done()` incondicional en el swap de pista sobrevivió a cambios repetidos de canción, con y sin carátula, donde la anterior producía `GPUCRASH`.

  **Causa raíz.** `Music_FreeCurrentTrack` **sí** llamaba a `vita2d_wait_rendering_done()`, pero desde dentro de la rama que libera la carátula, así que solo se ejecutaba cuando la pista saliente tenía arte embebida. Sin ese sync, el teardown —que bloquea el bucle de render largo rato: `Audio_Term` duerme 100 ms y la pista entrante decodifica su JPEG— ocurre con un frame todavía en vuelo, y el siguiente `vita2d_start_drawing` resetea el pool de vértices mientras la GPU aún lee de él.

  **Por qué apareció recién ahora:** antes de este change la interfaz ponía unas pocas reservas por frame en ese pool; dibujar cada ícono y cada esquina como vectores pone decenas, lo que ensancha muchísimo la ventana de riesgo. La carrera era latente, la vectorización la hizo alcanzable.

  **Arreglo (`f3d908e`):** el sync se sube al principio de `Music_FreeCurrentTrack`, fuera de la rama. Se eligió esa ubicación en vez de `Music_HandleNext` (que fue donde se probó) porque cubre la misma ventana y además la ruta del navegador: `Menu_PlayAudio` ejecuta el mismo teardown al abrir un archivo con otro ya sonando, y tenía el mismo agujero.
- **8.2 (FUERA DE ALCANCE — diferido a un change propio).** Texto borroso y poco definido. Reportado en hardware, pero **no es regresión de este change**: el diff `29903bc..HEAD` no toca carga de fuentes, atlas de glifos ni dibujo de texto (solo comentarios y constantes de color coinciden con la búsqueda). Viene de `add-ui-skin`. Puntos de partida: `UI_Theme_Load` carga las TTF con `vita2d_load_font_file`, se dibuja con `vita2d_font_draw_text`, y los tamaños en uso son `UI_FONT_SIZE_*` (11 a 22 px). Hipótesis a verificar, no confirmadas: el rasterizado de vita2d a esos tamaños pequeños sin hinting ni corrección de gamma, y/o el propio tamaño elegido para el panel de 960x544. Probablemente merezca su propio change.
- **8.3 (FUERA DE ALCANCE — diferido a un change propio).** Resto de los renders menos definidos de lo buscado (aunque el usuario los reporta como claramente mejores que antes). Es el riesgo de aliasing que `design.md` ya había marcado: las primitivas GXM no garantizan antialiasing, y los bordes diagonales de triángulos e íconos quedan "en escalera". Opciones a evaluar: MSAA vía `vita2d_init_advanced_with_msaa` (existe en el header, `vita2d.h:62`), o dibujar los bordes con un pequeño degradado.

## 7. Validación final

- [x] 7.1 Recorrer manualmente cada escenario de `specs/ui/dynamic-accent/spec.md` en hardware real (o el emulador usado por el proyecto) y confirmar que se cumple.
  - **Confirmado en hardware por el usuario (2026-09-17)**, sobre la build `f3d908e` instalada en la Vita: reproducción con carátula, cambios repetidos de pista entre distintas carátulas, y pistas sin carátula embebida, todo con comportamiento correcto.
  - **Alcance exacto de lo verificado, para que el registro no diga de más:** se confirmó el comportamiento observable de los escenarios. Los valores hexadecimales concretos de la tabla de predicción (i-dle `#EF3979`, Interdimensional Portal `#99EBD3`, sin carátula `#FF9166`) no se contrastaron uno a uno contra la pantalla. Esa predicción sí quedó verificada por otra vía: ejecutando el `ui_theme.c` real contra las carátulas reales extraídas de la tarjeta (tabla en 5.2).
  - Respaldo adicional sin dispositivo, ya registrado: trazado de los 5 escenarios contra el código (tabla en 6.2) y la suite del arnés de host (5.2).
- [x] 7.2 Confirmar que ningún requirement existente de `ui/nav-shell`, `ui/now-playing`, `ui/folder-browser` o `ui/settings` cambió de comportamiento observable como efecto colateral de la vectorización.
  - Se revisaron los 16 requirements de las 4 specs contra el diff completo. Ninguno especifica técnica de dibujo ni color de acento, y ninguno cambia de comportamiento observable. Los tres que sí tocan superficie modificada:
    - **`ui/nav-shell` — "Selecting a rail entry switches the active screen"**: solo se retiró el campo `icon` de `NavRail_Button` y se cambió el dibujo. Las cajas de hit test (`b->x`, `b->y`, `b->size`, con `RAIL_BUTTON_SIZE` 44 y `RAIL_SETTINGS_SIZE` 40) y las posiciones quedaron idénticas.
    - **`ui/now-playing` — "Playback transport controls"**: `Menu_HandleTransportTouch` no se tocó; `prev_x`, `next_x`, `side_y`, `shuffle_x`, `repeat_x` y `toggle_y` se siguen derivando de las mismas constantes. El glifo de shuffle/repeat ahora mide 22px dentro de la caja de 34px, mientras que la textura anterior medía 68px nativos y desbordaba esa misma caja: cambia lo que se ve, no dónde responde al toque.
    - **`ui/folder-browser` — "Docked mini-player while browsing"**: `Menu_HandleMiniPlayerTouch` intacto; los glifos se dibujan centrados en `prev_x + 15` y `next_x + 15`, los centros de las mismas cajas de 30px.
  - Los hit tests de `ui/settings` no se ven afectados: esa pantalla no hace hit testing táctil sobre sus controles (solo botones físicos más el nav rail), así que cambiar el tamaño del toggle de un lienzo de 60px a una pista de 48x26 no tiene consecuencias. Sí se desplaza levemente su posición visual (borde izquierdo de ~878 a 886, derecho de ~930 a 934); ningún requirement fija esa posición.
  - El acento pasa a ser dinámico, lo que cambia el color del indicador de pantalla activa, de los hints de Ajustes, etc. Eso es precisamente la capability nueva `ui/dynamic-accent`; ningún requirement existente nombra el color de acento, y "Rail indicates the active destination" se sigue cumpliendo (el marcado activo usa wash + color de ícono, ambos derivados del mismo acento).
  - **Riesgo no medible sin hardware:** la vectorización sube la cuenta de draw calls (un rect redondeado pasa de 4 quads texturados a 4 `vita2d_draw_array`; el engranaje del nav rail son 9 llamadas; cada `UI_DrawStroke` es 1 quad + 2 círculos). El orden de magnitud por frame en la pantalla más cargada (Ajustes) es ~100-150 llamadas, muy por debajo de lo que vita2d maneja, pero no se midió el frame time real. `design.md` ya había aceptado el costo de draw calls al evaluar la ruta alternativa.
