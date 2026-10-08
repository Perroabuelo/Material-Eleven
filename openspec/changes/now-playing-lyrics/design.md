## Context

Ver proposal.md - Why. Estado actual relevante:

- `source/menus/menu_audioplayer.c` dibuja Reproduciendo en un solo lazo (`Menu_RunNowPlayingLoop`). Cada pista se abre en `Menu_InitMusic` y se desmonta en `Music_FreeCurrentTrack` + `Audio_Term`; todo cambio de pista pasa por `Music_HandleNext`. Ese es el ciclo de vida que la letra tiene que seguir (`playback/track-lifecycle`).
- Cada decoder llena el struct global `metadata` en su `*_Init`: `flac.c` recorre los comentarios Vorbis, `ogg.c` usa `vorbis_comment_query`, `opus.c` usa `opus_tags_query`, y `mp3.c` recibe el `mpg123_id3v2` (que ya trae los marcos de texto convertidos a UTF-8). Ninguno lee la letra. Si `decoder.init` falla, `Audio_Init` pisa `metadata` con `empty_metadata` sin liberar nada.
- `Audio_GetPosition()` devuelve muestras decodificadas, y `Audio_GetPositionSeconds()` las divide por la frecuencia. No hay un accesor en milisegundos.
- `Audio_Seek(index)` recibe un píxel sobre una barra de 450 px (`total * index / 450.0` en cada decoder), y hoy la pantalla le pasa `Touch_GetX() - RIGHT_PANEL_X`.
- El texto se dibuja con `UI_DrawText` / `UI_TextWidth` en dos caras (Manrope y Plex Mono) y cinco tamaños (15 a 30 px). Un texto que la cara propia no cubre (hangul) pasa al fallback PVF, que se ve suave por encima de `UI_TS_FALLBACK_MAX` (19 px).
- `NavRail_DrawHints` deja de dibujar las entradas que no caben, desde la primera que no entra. Reproduciendo ya tiene seis.
- La lógica pura vive en `source/` sin llamadas a vita2d ni SCE y se prueba en `tests/` con ASan y UBSan. Los `.lrc` de prueba los genera `tools/testmedia/gen_testfiles.py` y ya están en la consola, en `ux0:/pruebas-eleven/letras/`.
- La colección real del usuario (revisada en la consola): 44 `.lrc` UTF-8 de LRCLIB, de 1,6 a 3,3 KB, con tiempos por línea, sin marcas por palabra ni `[offset:]`; 11 MP3 con `USLT` sin tiempos, de Genius, con encabezados `[Verse: …]`.

## Goals / Non-Goals

**Goals:**

- El parser, la detección de codificación y el ajuste de renglones son código puro, con tests en PC que cubren todos los `.lrc` de prueba.
- La letra sigue exactamente el ciclo de vida de la pista: se toma en `Menu_InitMusic` y se libera en `Music_FreeCurrentTrack`, también cuando la pista no abre.
- Dibujar la vista de letras no lee el disco, no reserva memoria y no mide texto que no se va a ver: todo eso se hace al abrir la pista.
- La vista normal de Reproduciendo no cambia.

**Non-Goals:**

- Cambiar la unidad de `Audio_Seek` (queda para el cambio de la barra de progreso).
- Animaciones más allá del desplazamiento suave de la letra.
- Guardar en `config.cfg` qué vista estaba abierta.

## Decisions

### 1. Tres módulos puros y uno con E/S

```
 source/text_encoding.c  (puro)  bytes -> UTF-8 valido
 source/lyrics.c         (puro)  UTF-8 -> Lyrics { synced, lines[] }
 source/lyrics_layout.c  (puro)  Lyrics + ancho + medida -> renglones
 source/lyrics_load.c    (SCE)   ruta + texto embebido -> Lyrics*
```

- `TextEncoding_ToUtf8(const unsigned char *in, size_t len, char **out, size_t *out_len)`: las cuatro capas de la spec. La validación UTF-8 es estricta: rechaza secuencias sobrelargas, sustitutos y valores mayores que U+10FFFF. La tabla de Windows-1252 cubre 0x80-0x9F (los cinco huecos de la tabla pasan a U+FFFD), y 0xA0-0xFF se mapea directo. Un UTF-16 con un sustituto huérfano produce U+FFFD. La salida siempre termina en `\0`.
- `Lyrics_Parse(const char *utf8, size_t len, Lyrics *out)`: separa líneas por `\n`, `\r\n` y `\r`; reconoce las marcas de la spec y las etiquetas conocidas; quita las marcas por palabra; aplica `[offset:]`; ordena por tiempo con un ordenamiento estable (para que dos líneas con la misma marca conserven su orden del archivo); y marca como encabezado una línea que empieza con `[` y termina con `]`. Si no hay ninguna marca válida, la letra queda sin sincronizar y conserva todas las líneas. Si no queda ninguna línea con texto, devuelve "sin letra".
- `Lyrics_LineAt(const Lyrics *, uint32_t ms)` devuelve la última línea cuya marca es menor o igual que `ms`, o -1 si todavía no hay ninguna. Usa búsqueda binaria y se llama una vez por fotograma.
- `LyricsLayout_Build(const Lyrics *, float width, float (*measure)(const char *, size_t, uint32_t line, void *), void *ctx, LyricsLayout *out)`: parte cada línea en renglones en los espacios. Una palabra más ancha que la vista se corta entre puntos de código, y nunca a mitad de una secuencia UTF-8. El hangul y el kanji no siempre separan con espacios, así que se permite cortar entre dos caracteres CJK. Guarda, por renglón, el índice de su línea y su rango de bytes. La medida se inyecta y recibe el índice de la línea, para que la consola elija el tamaño de cada línea (con o sin fallback) y todos sus renglones lo compartan: en la consola es `UI_TextWidth`, y en PC un ancho fijo por punto de código.

*Alternativa descartada:* un solo módulo que lea el archivo y dibuje. No se podría probar en PC, y las reglas del proyecto piden que el parsing sea puro.

### 2. Representación en memoria: un bloque por letra

`Lyrics` guarda el texto decodificado en un solo buffer, y `lines[]` es un arreglo de `{ uint32_t ms; uint32_t off; uint32_t len; uint8_t flags; }` que apunta dentro de él (las marcas por palabra se quitan copiando dentro del mismo buffer). Una línea con varias marcas genera varias entradas que apuntan al mismo texto. El layout tiene su propio arreglo de renglones. En total son tres reservas por pista (texto, líneas, renglones), y `Lyrics_Free` / `LyricsLayout_Free` las devuelven.

Cuentas con el peor caso permitido: 64 KB de texto, menos de 64 K líneas, y unos pocos KB de líneas y renglones para una letra real. Para una letra típica (3 KB, 60 líneas, unos 120 renglones) son menos de 8 KB.

### 3. La letra embebida pasa por `metadata`

`Audio_Metadata` gana `char *lyrics; size_t lyrics_len;`. Cada decoder copia la letra embebida con `malloc` si existe y mide 64 KB o menos:

- FLAC: comentario `LYRICS=` o `UNSYNCEDLYRICS=` (el primero que aparezca), en el bucle que ya recorre los comentarios.
- OGG: `vorbis_comment_query(c, "LYRICS", 0)` y si no, `"UNSYNCEDLYRICS"`.
- Opus: `opus_tags_query` con las mismas dos claves.
- MP3: el primer `v2->text[i]` con `id` igual a `"USLT"` y texto no vacío, desde `text.p`, que mpg123 ya entrega en UTF-8. Esto se comprueba en la consola con un `USLT` en coreano (ver Riesgos).

*Quién libera:* `Menu_InitMusic` toma el puntero justo después de `Audio_Init` y deja `metadata.lyrics = NULL`. A partir de ahí la letra pertenece al módulo de carga. Si `decoder.init` falla después de haber copiado la letra, `Audio_Init` la libera antes de pisar `metadata` con `empty_metadata`. `Audio_Term` también libera `metadata.lyrics` si sigue ahí, por si alguna ruta no pasa por `Menu_InitMusic`.

*Alternativa descartada:* releer el archivo con `tags.c`. Se abriría cada pista dos veces, y `tags.c` está pensado para el escaneo de la biblioteca.

### 4. Carga y elección de fuente en `lyrics_load.c`

`LyricsLoad_ForTrack(const char *path, char *embedded, size_t embedded_len, Lyrics *out)`:

1. Arma la ruta del `.lrc` reemplazando la extensión. Lo abre con `sceIoOpen`, mira el tamaño con `sceIoLseek`, y si mide más de 64 KB lo descarta sin leerlo. Si no, lo lee entero, lo pasa por `TextEncoding_ToUtf8` y por `Lyrics_Parse`.
2. Interpreta el texto embebido con `Lyrics_Parse`. Va directo, sin pasar por `TextEncoding`, porque ya es UTF-8. Para no confiar en el archivo, igual se valida, y una secuencia inválida se reemplaza por U+FFFD.
3. Se queda con una de las dos según la regla de la spec. La elección es una función pura, `Lyrics_Choose(a, b)`, y libera la descartada.
4. Libera `embedded` en todos los casos, aunque falle a mitad de camino: el buffer leído del archivo se libera al terminar de convertirlo, y el convertido al terminar de parsearlo.

Se llama desde `Menu_InitMusic`, después de abrir la pista. Lee un archivo de unos 3 KB por pista, y lo hace dentro de la pausa que el cambio de pista ya tiene (el mismo lugar donde hoy se precalientan los glifos del título).

El layout se arma en el mismo momento, con el ancho de la vista de letras, y las líneas se pasan por `UI_TextWidth` para precalentar el atlas de glifos del fallback. Así el primer fotograma de la vista no se traba con texto coreano.

### 5. Posición en milisegundos

Se añade `Audio_GetPositionMs()`, que calcula `posición * 1000 / frecuencia` con aritmética de 64 bits, y no toca los decoders.

La posición que dan los decoders es la de las muestras decodificadas, que va por delante de lo que se oye en uno o dos bloques de salida (960 muestras por defecto, unos 20 ms a 48 kHz). Eso es imperceptible para una línea de letra, así que no se compensa.

### 6. La vista de letras dentro del mismo lazo

En `menu_audioplayer.c`, una variable `static SceBool lyrics_view` alterna con `SCE_CTRL_UP`. No se crea una pantalla nueva ni un `UI_Screen` nuevo, porque la vista es parte de Reproduciendo y el nav rail no debe cambiar. El lazo dibuja `Menu_DrawLyricsView()` o el contenido actual según la variable. Los botones físicos se procesan igual en las dos vistas, y el toque en el transporte se procesa solo en la vista normal.

La disposición, dentro del área de contenido (de `UI_RAIL_WIDTH` a 960, y de `STATUS_H` a `544 - UI_HINT_BAR_HEIGHT`):

```
 cabecera  (56 px): caratula 40 px | titulo UI_TS_BODY + artista UI_TS_LABEL | badge
 letra     (resto): ancho = contenido - 2 * 48 px de margen, recortada con el clip de vita2d
 progreso  (40 px): barra a todo el ancho + tiempos UI_TS_LABEL a los lados
```

- **Tamaños:** la línea resaltada va en `UI_TS_TITLE` (22 px) y color de texto primario; las demás en `UI_TS_BODY` y color terciario. Si una línea necesita el fallback, todos sus renglones van en `UI_TS_FALLBACK_MAX`, y se distingue solo por el color (spec `ui/typography`: nada se reescala). Los encabezados de una letra sin tiempos van en color atenuado. El layout se arma con el ancho medido al tamaño mayor de cada línea, para que resaltarla no cambie cuántos renglones ocupa.
- **Desplazamiento:** el centro deseado es el centro del primer renglón de la línea actual. La posición mostrada se acerca a él con un filtro exponencial por fotograma (`y += (destino - y) * 0.2`), y se detiene al estar a menos de medio píxel. Lo mismo vale para el regreso después de arrastrar.
- **Arrastre:** mientras `Touch_CheckHeld()` está activo dentro del área de la letra, el desplazamiento es la diferencia de Y respecto del fotograma anterior, acotado para que el primer y el último renglón no pasen del centro. Al soltar se guarda el instante con `sceKernelGetProcessTimeWide()`. Pasados 3 s sin tocar, se vuelve a seguir la canción. Una letra sin tiempos nunca vuelve sola.
- **Barra de progreso de la vista:** mide unos 800 px, así que el toque se escala a la escala de 450 que espera `Audio_Seek`: `index = (x - bar_x) * 450 / bar_w`. La vista normal no se toca. Su barra mide 426 px (de x = 500 a x = 926) y le pasa el píxel sin escalar, así que tocar el final de esa barra lleva al 95 % de la canción. Es un error que ya existe y queda anotado para el cambio de la barra de progreso.
- **Cambio de pista:** el desplazamiento se reinicia y la vista vuelve al seguimiento. `lyrics_view` no se toca.

### 7. Leyenda

Se añade `HINT_BTN_UP` a `NavRail_HintButton`. Se dibuja como chip neutro (`HINT_SYM_CHIP`) con una flecha hecha con `UI_DrawTriangle`, en lugar del nombre, y su ancho es el de un chip de una letra.

En Reproduciendo, la entrada "Letras" / "Lyrics" va después de L/R y antes de Triángulo, porque la barra corta desde el final. Textos nuevos en `lang_strings.h`: `STR_HINT_LYRICS` ("Lyrics" / "Letras") y `STR_NO_LYRICS` ("No lyrics" / "Sin letra").

### Estrategia de pruebas

- **Compilación** limpia con `-Wall -Werror` (CI).
- **Tests en PC** (`make -C tests`, con ASan y UBSan):
  - `test_text_encoding`: los cuatro caminos, una secuencia sobrelarga, sustitutos, UTF-16 con un sustituto huérfano, una entrada vacía, una entrada de un solo byte, y 10 000 bloques de bytes pseudoaleatorios con semilla fija. En todos se verifica que la salida sea UTF-8 válido.
  - `test_lyrics`: cada escenario de la spec `playback/lyrics` con los `.lrc` de `tests/fixtures/lyrics/`, `Lyrics_LineAt` en los bordes (antes de la primera línea, exactamente en una marca y después de la última), y la regla de elección entre fuentes.
  - `test_lyrics_layout`: cortes en espacios, una palabra más larga que el ancho, texto CJK sin espacios, que nunca se corte dentro de una secuencia UTF-8, y una línea vacía.
- **Fixtures:** `gen_testfiles.py` gana una opción para escribir solo los `.lrc` en `tests/fixtures/lyrics/`, y se versionan. Se suman los casos nuevos: UTF-16 LE/BE, `[offset:]`, desorden, marcas por palabra, una letra de estilo Genius y una de más de 64 KB que se genera al vuelo, sin versionarla. Los mismos casos se escriben en la carpeta de la consola.
- **Consola:** los escenarios de las specs, paso a paso, con la colección real (`I NEVER DIE - i-dle`, *KPop Demon Hunters*) y `ux0:/pruebas-eleven/letras/`. Además, el escenario de heap con el overlay de debug, y la leyenda en los dos idiomas para confirmar que no se corta ninguna entrada.

### CI

No cambia el workflow. Los tests nuevos entran por `tests/Makefile`, que CI ya corre, y los fixtures quedan versionados en el repo.

### Licencias

No se agrega código ni assets de terceros. La tabla de Windows-1252 es un dato público (la tabla de mapeo de Unicode) y se escribe a mano.

## Risks / Trade-offs

- [mpg123 podría no entregar `USLT` en `v2->text`, o no convertirlo a UTF-8] → Lo primero que se hace en la tarea de MP3 es comprobarlo en la consola con *Golden*, que trae hangul. Si mpg123 lo deja en la codificación original, se usa el campo de codificación de `mpg123_text` y `TextEncoding_ToUtf8` con el BOM que corresponda.
- [La leyenda tiene siete entradas y en español puede no caber la última ("Apagar pantalla")] → "Letras" va antes de Triángulo y la prueba en consola recorre los dos idiomas. Si START queda fuera, se acortan textos de la leyenda de esta pantalla, sin quitar entradas.
- [Un `.lrc` en Shift-JIS o GBK se verá con caracteres incorrectos, porque se toma como Windows-1252] → Es una limitación aceptada (proposal, Fuera de alcance). El README lo avisa, y la salida sigue siendo UTF-8 válido, así que no hay errores de dibujo.
- [Leer el `.lrc` alarga el cambio de pista] → Son unos 3 KB desde la tarjeta, dentro de una pausa que ya existe. El tope de 64 KB acota el peor caso, y el tamaño se mira antes de leer.
- [Medir todos los renglones al abrir la pista cuesta tiempo con texto CJK nuevo] → Es el mismo costo que hoy tiene el precalentamiento del título, y una letra típica tiene unos 60 renglones. Si se nota en la consola, se mide solo la línea actual y sus vecinas, y el resto se mide a medida que se necesita.
- [Tener `lyrics_view` solo en memoria hace que la vista se pierda al cerrar la app] → Es lo decidido. La spec dice que la app arranca siempre en la vista normal.
- [La posición puede ir 20-40 ms por delante de lo que se oye] → No se nota en una línea de letra. Si `[offset:]` se usa en la práctica, también lo compensa.

## Migration Plan

No hay migración: no cambian `config.cfg` ni el índice de la biblioteca. La versión sube a v3.5.0 con su entrada en CHANGELOG. Para volver atrás basta con instalar el `.vpk` anterior.
