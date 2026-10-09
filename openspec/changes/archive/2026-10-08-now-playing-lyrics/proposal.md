## Why

Reproduciendo muestra la carátula, el título y el transporte, pero no la letra de la canción, aunque la colección del usuario ya la tiene: en la consola hay 44 archivos `.lrc` sincronizados (descargados de LRCLIB, junto a sus FLAC) y 11 MP3 con la letra embebida en un tag `USLT`. Hoy esa letra no se ve en ninguna parte de la app.

## What Changes

- **Vista de letras en Reproduciendo.** D-pad arriba alterna entre la vista actual y una vista de letras a pantalla completa (dentro de Reproduciendo, con el nav rail y la leyenda de botones). La vista muestra una cabecera compacta con la carátula pequeña, el título, el artista y el badge de formato, la letra en el centro y la barra de progreso abajo.
- **Letra sincronizada por línea.** Con una letra con tiempos, la línea que suena se resalta y queda centrada, con las anteriores y las siguientes atenuadas alrededor, y la vista avanza sola con la canción.
- **Desplazamiento táctil.** Arrastrar sobre la letra la desplaza y suelta el seguimiento automático; unos segundos después de soltar, la vista vuelve sola a la línea que suena.
- **Letra sin tiempos.** Una letra sin marcas de tiempo se muestra como texto estático que se desplaza con el tacto. Los encabezados de sección (por ejemplo `[Verse: Rumi]`) se muestran atenuados.
- **Dos fuentes de letra.** Un archivo `.lrc` con el mismo nombre que la pista, en la misma carpeta, y la letra embebida en la pista (`LYRICS` o `UNSYNCEDLYRICS` en FLAC, OGG y Opus; `USLT` en MP3). Una letra con tiempos gana a una sin tiempos; a igualdad, gana el `.lrc`.
- **Sin letra.** Si la pista no tiene letra, la vista muestra "Sin letra" / "No lyrics".
- **Codificación de los `.lrc` detectada por capas.** BOM de UTF-8 o UTF-16; si no hay BOM, UTF-8 si el archivo es UTF-8 válido; si no, Windows-1252. El README avisa que un `.lrc` en otra codificación (por ejemplo Shift-JIS o GBK) se verá con caracteres incorrectos y que conviene guardarlos en UTF-8.
- **Leyenda.** Reproduciendo añade a la leyenda la entrada de D-pad arriba para abrir y cerrar las letras.

## Capabilities

### New Capabilities

- `playback/lyrics`: de dónde sale la letra de una pista (`.lrc` o tag embebido) y cuál gana, cómo se interpreta el formato LRC (tiempos por línea, `[offset:]`, etiquetas de metadatos, marcas por palabra que se ignoran), cómo se detecta la codificación de un `.lrc`, el límite de tamaño, y que la letra se libera con la pista.

### Modified Capabilities

- `ui/now-playing`: se agrega la vista de letras, su toggle con D-pad arriba, el seguimiento de la línea que suena, el desplazamiento táctil con regreso automático, la letra estática y el aviso de "Sin letra".
- `ui/nav-shell`: la leyenda de botones admite el D-pad arriba como un chip neutro, igual que L, R, SELECT y START.

## Impact

- **Código nuevo, puro y probado en PC:** `source/lyrics.c` (parser LRC y letra sin tiempos), `source/text_encoding.c` (detección y conversión a UTF-8) y el ajuste de líneas al ancho de la vista, con una función de medida inyectada. Tests nuevos en `tests/`.
- **Carga:** lectura del `.lrc` hermano con las funciones de E/S de la consola, y captura del tag embebido en `source/audio/flac.c`, `ogg.c`, `opus.c` y `mp3.c` (`include/audio/audio.h` gana un campo para el texto crudo de la letra).
- **Audio:** un accesor de posición en milisegundos en `source/audio/audio.c`.
- **UI:** `source/menus/menu_audioplayer.c` (vista de letras, toggle, desplazamiento), `source/nav_rail.c` e `include/nav_rail.h` (chip de D-pad arriba), `include/lang_strings.h` (textos nuevos en inglés y español).
- **Memoria:** la letra se reserva al abrir la pista y se libera junto con ella; el límite es 64 KB por letra.
- **Documentación:** README (fuentes de letra, nota sobre UTF-8) y CHANGELOG.
- Sin dependencias ni licencias nuevas. Sin cambios en CI más allá de los tests nuevos en `tests/Makefile`.

## Rama

`change/now-playing-lyrics`

## Fuera de alcance

- **Descargar letras de internet.** No se hará en este cambio ni está previsto.
- **Sincronización por palabra (karaoke, LRC A2).** Las marcas por palabra se descartan y se usa el tiempo de la línea.
- **Tocar una línea para saltar a ese momento.** Queda para el cambio que rehaga la barra de progreso, porque hoy el seek recibe un píxel sobre una barra fija de 450 px y no un tiempo.
- **Letra sincronizada en `SYLT` (ID3 binario).** Solo se lee `USLT`.
- **Convertir codificaciones CJK antiguas** (Shift-JIS, GBK, Big5, EUC-KR). No se pueden distinguir de forma fiable y requieren tablas grandes; el README pide UTF-8.
- **Ajuste manual del desfase** de la letra desde la app. Solo se respeta `[offset:]` si el archivo lo trae.
- **Letras en el mini reproductor** o en otras pantallas.
- **Letras para módulos tracker** (MOD, XM, IT, S3M), más allá de un `.lrc` hermano si existiera.
- **Indexar letras en la biblioteca** o buscar por letra.

## Notas de version

Versión objetivo: **v3.5.0** (minor: funcionalidad nueva).

- Now Playing can show the song's lyrics: press Up on the D-pad to open a full-screen lyrics view, and again to close it.
- Synced lyrics follow the song line by line, with the current line highlighted.
- Drag the lyrics to look around; after a few seconds the view returns to the line being sung.
- Lyrics are read from a `.lrc` file next to the track (same name) or from the lyrics embedded in the file (FLAC, OGG and Opus `LYRICS`, MP3 `USLT`).
- Lyrics without timestamps are shown as scrollable text, and tracks without lyrics say so.
- `.lrc` files should be saved as UTF-8; files in other encodings such as Shift-JIS or GBK may show wrong characters.
