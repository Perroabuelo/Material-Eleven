## Context

La motivación está en `proposal.md` (Why) y el comportamiento en las delta specs (`library/playlists`, `library/views`, `ui/nav-shell`). Lo que condiciona el cómo:

- **La cola ya acepta cualquier lista.** `Queue_Clear` + `Queue_Add(path, title, album)` + `Queue_SeekToPath` es exactamente lo que hace `Menu_LibraryPlaySelected` con una vista. Una playlist se reproduce igual, sin tocar `queue.c`.
- **La pantalla de biblioteca es un estado estático que sobrevive a salir de ella.** `menu_library.c` guarda `view` (enum `Menu_LibraryTab`, hoy 4 valores), `selection`, `inside`/`inside_name`, los dos `ListFilter` y los dos términos. "Volver a la misma fila" desde Reproduciendo funciona porque nada de eso se reinicia.
- **El menú de Cuadrado es fijo.** `OPTION_RESCAN`/`OPTION_CHOOSE_FOLDER` en un enum, dibujado en un recuadro sobre el mini reproductor y atendido dentro del mismo lazo (`options_open`).
- **El aviso solo admite un texto fijo.** `Menu_LibraryNotice(LangString)` lo muestra durante `NOTICE_FRAMES`. Los avisos nuevos llevan nombre y números.
- **El caché de carátulas.** `cover.c` guarda en `ux0:data/ElevenMPV/covers/<hash>.cv` una cabecera de 128 bytes y los píxeles RGBA ya reducidos a 128×128 (64 KB). La clave es el álbum, o la ruta si no hay álbum. En RAM caben `COVER_CACHE_SLOTS` (16) texturas. `Cover_Get` tiene un presupuesto de lecturas por fotograma y `Cover_Probe` lee solo la cabecera.
- **El índice no tiene búsqueda por ruta.** `Library_GetTrack(i)` es por posición. Saber si una ruta de la playlist sigue en la biblioteca, comparando contra 4000 entradas para cada una de hasta 4000 canciones, es demasiado lento sin una estructura.
- **Diálogo de texto, plegado y filtros existen y son puros donde importa:** `SearchDialog_Prompt`, `TextFold_Fold`/`TextFold_Contains`, `ListFilter_*` y `TextEncoding_*`.
- **`LIST_FILTER_MAX >= LIBRARY_MAX_TRACKS`** (4000), que es también el techo de la cola y el de una playlist.

## Goals / Non-Goals

**Goals:**
- Todo el modelo de playlist (formato, operaciones, reglas de nombre, elección del mosaico, recuperación tras un corte) como código puro con tests en PC.
- Ninguna operación de la UI que dependa de tener todas las playlists completas en RAM a la vez.
- El mosaico cuesta una textura por fila, como cualquier carátula.

**Non-Goals:**
- Cambiar `queue.c`, el orden de ninguna vista o el formato del índice.
- Leer playlists ajenas. El lector tolera lo que un usuario podría editar a mano en un archivo nuestro (BOM, CRLF, líneas vacías, comentarios), pero no resuelve rutas de PC.
- Un componente de diálogos genérico para toda la app. El diálogo de confirmación vive en la biblioteca.

## Decisions

### 1. Tres módulos: modelo puro, almacenamiento y pantalla

```
 playlist.c (puro, tests/test_playlist.c)
   Playlist { name, entries[] {path,title,artist,album}, count, cap }
   Playlist_Parse(buf,len) / Playlist_Serialize(pl) -> texto .m3u8
   Playlist_Append(pl, entries, n, &added, &skipped, &full)  sin duplicados, techo 4000
   Playlist_Remove(pl, i) / Playlist_Move(pl, from, to)
   Playlist_NameValid / Playlist_NamesEqual (TextFold) / Playlist_FileName (saneado)
   Playlist_MosaicAlbums(pl, probe_cb, out[4]) -> 0..4
   Playlist_Recover(exists_main, exists_tmp, exists_bak) -> que hacer

 playlist_store.c (SCE, sceIo*)
   PlaylistStore_List(summaries)  / Load(file) / Save(pl) / Rename / Delete

 menu_library.c
   VIEW_PLAYLISTS, nivel "dentro de una playlist", menu contextual,
   selector, modo mover, confirmacion, avisos con formato
```

`playlist.c` no incluye cabeceras SCE ni vita2d, igual que `queue.c`, `list_filter.c` o `lyrics.c`. Para elegir el mosaico recibe una función que dice si un álbum tiene carátula, de modo que el test la reemplaza por una tabla.

*Alternativa descartada:* meter las playlists en `library.c`. Ese módulo incluye SCE y no se prueba en PC, y ya mezcla índice, escaneo y vistas.

### 2. Formato del archivo

```
#EXTM3U
#PLAYLIST:Viaje a la costa
#EXTINF:-1,Daft Punk - Digital Love
#EXTALB:Discovery
ux0:music/Daft Punk/Discovery/03 Digital Love.flac
```

- UTF-8 sin BOM, `\n`. El lector acepta BOM y `\r\n`, ignora líneas vacías y las `#` que no reconoce. Una ruta sin `#EXTINF` previo se acepta con título vacío.
- `#EXTINF` lleva duración `-1`, que el estándar admite como "desconocida", porque el índice no guarda duración. El texto es `Artista - Título`. Al leer se separa por el primer `" - "`. Si el artista contiene `" - "` el corte cae mal, pero solo afecta a la fila atenuada de una canción que ya no está en la biblioteca. Las disponibles muestran los tags del índice.
- Un archivo sin `#EXTM3U` o sin ninguna entrada válida ni `#PLAYLIST` se considera inválido y se ignora, como pide la spec.

### 3. Nombre de archivo y renombrado

- **Nombre de archivo:** sale del nombre visible. Se reemplazan `/ \ : * ? " < > |` y los caracteres de control por `_` y se recorta a 64 bytes UTF-8 sin partir un carácter. Si ya existe otro archivo con ese nombre (por ejemplo `a/b` y `a_b`), se agrega ` (2)`, ` (3)`… El nombre visible siempre es el de `#PLAYLIST`.
- **Renombrar se hace en dos pasos, cada uno seguro:** primero se guarda el contenido con el `#PLAYLIST` nuevo (Decisión 4) y después `sceIoRename` cambia el nombre del archivo. Un corte entre ambos deja un archivo con el nombre viejo y el contenido nuevo, que es una playlist válida con el nombre nuevo.
- **Unicidad:** se valida con `Playlist_NamesEqual`, que pliega con `TextFold_Fold` (mayúsculas, tildes, ñ). Es la misma regla que la búsqueda, y además coincide con que el sistema de archivos de la Vita no distingue mayúsculas.

### 4. Guardado a prueba de cortes

`sceIoRename` en la Vita no reemplaza un destino que existe, así que el guardado es:

```
 1. escribir  X.m3u8.tmp   (completo, cerrar)
 2. renombrar X.m3u8      -> X.m3u8.bak     (si X existe)
 3. renombrar X.m3u8.tmp  -> X.m3u8
 4. borrar    X.m3u8.bak
```

Al listar la carpeta, `Playlist_Recover` decide qué hacer según qué archivos de la terna existen:

| main | tmp | bak | Acción |
|---|---|---|---|
| sí | sí | – | Borrar el tmp (corte en 1) |
| no | sí | sí | Renombrar el bak a main y borrar el tmp (corte entre 2 y 3, vuelve a la versión anterior) |
| sí | – | sí | Borrar el bak (corte en 4) |

En ningún caso queda una playlist a medias ni vacía. La tabla se prueba en PC.

### 5. Orden por modificación sin campo propio

La fecha de modificación del archivo (`st_mtime` de `sceIoDread`) es la fecha de la playlist. Guardar crea un archivo nuevo en el paso 3, así que toda modificación la pone al frente. Reproducir no escribe nada. No hace falta guardar una marca de tiempo propia en el archivo.

### 6. Pestaña Playlists: resúmenes, no playlists completas

Al entrar en la pestaña (o tras una modificación propia) se lista la carpeta y se lee cada archivo una vez para sacar su resumen:

```
 PlaylistSummary { file[], name[65], count, mtime, mosaic_albums[4] (y cuantos) }
```

- Hay un arreglo de resúmenes, ordenado por `mtime` descendente. La lectura de cada archivo se libera apenas sale su resumen.
- La playlist completa (hasta 4000 × unos 450 B, alrededor de 1,8 MB en el peor caso) solo se carga al entrar en ella. Para añadir desde otra vista se carga la playlist destino, se le añade, se guarda y se libera.
- Los resúmenes se rehacen solo cuando esta pantalla escribe una playlist, no en cada fotograma.

*Alternativa descartada:* cachear `count` y los álbumes del mosaico en una línea propia del archivo. Ahorraría leer el archivo entero, pero introduce un dato que puede quedar desincronizado si el usuario edita el archivo a mano. Leer 20 playlists de tamaño normal al entrar en la pestaña es barato. Ver Riesgos.

### 7. Disponibilidad: índice ordenado por ruta

Después de `Library_Load` y de cada escaneo se construye, una sola vez, un arreglo de índices de pista ordenado por ruta (`qsort`). `Library_FindPath(path)` hace búsqueda binaria. Una canción de la playlist está disponible si su ruta está en la biblioteca, y entonces la fila usa los tags y la extensión del índice. Si no está, la fila se atenúa y usa título y artista guardados.

- La ordenación y la búsqueda se escriben como funciones puras sobre un arreglo de cadenas, con test en PC. `library.c` solo las llama.
- No se consulta el sistema de archivos por cada fila. Un archivo borrado sin reescanear aparece disponible y se salta al reproducir, como define `playback/queue` y como dice la spec.

### 8. Mosaico: una textura compuesta y direccionada por contenido

```
 Playlist_MosaicAlbums -> [A, B, C, D]           (primeros 4 albumes distintos
                                                  con caratula; Cover_Probe)
        |
        v  clave = "\x01mosaic:" + hash(A) + hash(B) + hash(C) + hash(D)
 Cover_GetMosaic(clave, albums, n, &budget)
        | entrada en cache?  -> textura (1 slot, como un album)
        | si no y hay budget: leer A (64 KB) -> reducir 2x -> cuadrante 0
        |                     leer B ...                  -> cuadrante 1 ...
        |                     guardar entrada .cv con la clave -> textura
        v
   n = 1..3 -> Cover_Get(A) directo, sin componer;  n = 0 -> marcador
```

- **Clave por contenido.** Si cambian los cuatro primeros álbumes, cambia la clave y el mosaico se recompone solo, sin lógica de invalidación. Renombrar no lo invalida, y dos playlists que empiezan igual lo comparten. El `\x01` evita que choque con un álbum real.
- **La reducción 2×** (promedio de 2×2 píxeles RGBA) va en una función pura con test en PC. La composición lee las cuatro entradas de una en una sobre un único buffer de 64 KB y escribe en otro de 64 KB.
- **Componer cuesta un presupuesto de `Cover_Get`**, así que la lista no se detiene. Las filas sin mosaico listo muestran el marcador y lo reciben en un fotograma posterior.
- **Solo se guarda un mosaico definitivo.** Si alguno de los álbumes candidatos todavía no pasó por la pasada de carátulas (`COVER_PROBE_MISSING`), se muestra lo que haya pero no se guarda la entrada, para que se componga bien después.
- **Mosaicos huérfanos.** Al recomponer, se borra la entrada de la clave anterior que tenía el resumen. Los huérfanos que queden por un corte ocupan 64 KB cada uno y no afectan al comportamiento.

### 9. Pantalla: pestaña, nivel interior y menús

- `VIEW_PLAYLISTS` se agrega al final del enum. L/R ya envuelven por `VIEW_COUNT`.
- **"Dentro de una playlist"** reutiliza `inside` y `group_filter`/`group_search`, con un `inside_kind` (artista, álbum, playlist) y el nombre de archivo de la playlist abierta en lugar de `inside_name`. Volver desde Reproduciendo recarga esa playlist por nombre de archivo. Si ya no existe, queda en la pestaña sin entrar, como dice la spec de `ui/nav-shell`.
- **El menú de Cuadrado** pasa de un enum fijo a una lista de hasta 4 opciones que se arma al abrirlo según la fila marcada. Las acciones de playlist van primero y reescanear y elegir carpeta al final. Dibujo e input no cambian: el recuadro crece con la cantidad de opciones.
- **El selector de playlists** es el mismo recuadro con "Nueva playlist…" más los resúmenes, recorrido con `Utils_ListStep` y con desplazamiento si no caben. "Nueva…" llama a `SearchDialog_Prompt` con el título "Nombre de la playlist".
- **El modo mover** es un estado del lazo. Mueve el puesto en una copia de trabajo de la playlist abierta y solo al confirmar guarda. El botón de volver restaura el puesto original.
- **El diálogo de confirmación** usa el mismo recuadro con una línea de título y dos opciones, con "Cancelar" marcada al abrir.
- **El aviso** pasa a guardar un texto ya formateado (`char notice_text[128]`) en lugar de un `LangString`. `Menu_LibraryNotice` mantiene su firma para los avisos fijos y se suma `Menu_LibraryNoticef`.

### 10. Reproducir una playlist

`Queue_Clear`, y para cada entrada `Queue_Add(path, title_mostrado, album)`, donde `album` sale del índice si está disponible o de `#EXTALB` si no, para que "A continuación" muestre su carátula. Después `Menu_PlayQueued(path_elegida)`. Como la cola copia las cadenas, editar o borrar la playlist después no la toca: eso es la "foto" que pide la spec, sin código extra.

### 11. Recursos: quién libera qué

| Recurso | Lo toma | Lo libera | Si algo falla a mitad |
|---|---|---|---|
| Playlist abierta (entradas) | Entrar en una playlist | Salir de ella, cambiar de pestaña, salir de la pantalla (`Menu_LibraryLeave`) | `Playlist_Parse` libera lo parcial y devuelve error. La pantalla muestra la pestaña sin entrar. |
| Playlist destino de "Añadir" | `PlaylistStore_Load` | Al final de la acción, con éxito o sin él | Cada salida de error de `Save` pasa por el mismo `Playlist_Free`. |
| Resúmenes de la pestaña | Entrar en la pestaña o escribir | Salir de la pestaña o de la pantalla | Un archivo que falla se salta; el arreglo parcial es válido. |
| Buffer de lectura de archivo | `PlaylistStore_*` | La misma función, antes de volver | Cierre del fd y `free` en un único punto de salida. |
| Buffers del mosaico (2 × 64 KB) | `Cover_GetMosaic` | La misma llamada | Una entrada ilegible deja su cuadrante con el color de superficie y el mosaico no se guarda. |
| Textura del mosaico | `Cover_GetMosaic` | El LRU de `cover.c` y `Cover_Free`, como cualquier carátula | Sin cambios. |
| Índice por ruta | `Library_Load` y el fin del escaneo | `Library_Free` y el siguiente rebuild | Si falla la reserva, `Library_FindPath` responde "no está" y las filas se ven atenuadas pero reproducen. |
| Archivos `.tmp`/`.bak` | `PlaylistStore_Save` | El mismo guardado, o `Playlist_Recover` al listar | Ver la Decisión 4. |

No se crean hilos.

## Risks / Trade-offs

- **[Cinco pestañas y la caja de búsqueda pueden no caber en la barra superior a 960 px, sobre todo en español.]** → La etiqueta es "Playlists" en los dos idiomas. La primera tarea de UI mide el ancho en consola y, si no cabe, reduce la separación entre pestañas antes de tocar la caja.
- **[Una playlist enorme hace lenta la entrada a la pestaña, porque se lee el archivo entero para el resumen.]** → El peor caso razonable (pocas playlists de miles de canciones) se mide en consola en la tarea de la pestaña. Si pasa de unos 300 ms, se cachean los resúmenes en un archivo de índice derivado, que puede reconstruirse siempre a partir de los `.m3u8`.
- **[Un `" - "` dentro del nombre de un artista parte mal el `#EXTINF`.]** → Solo afecta a filas atenuadas. Las disponibles usan el índice.
- **[La comprobación de duplicados al añadir un artista grande a una playlist grande es cuadrática (500 × 4000).]** → Son unos 2 M de `strcmp`, decenas de ms una sola vez por acción. Si la consola lo nota, la playlist destino se ordena por ruta en una copia temporal y se busca en ella.
- **[Mosaicos huérfanos en el caché de carátulas.]** → 64 KB cada uno, solo si hay un corte durante la recomposición. Una limpieza futura de `covers/` puede recogerlos.
- **[El usuario edita a mano un `.m3u8` y lo rompe.]** → El archivo inválido se ignora sin cerrar la app, como pide la spec. No se borra, así que puede repararlo.

## Migration Plan

No hay datos previos que migrar: la carpeta `playlists/` se crea al guardar la primera playlist. Un usuario que vuelve a una versión anterior no ve sus playlists, pero los archivos siguen en la tarjeta y reaparecen al actualizar. No cambia el formato del índice ni de `config.cfg`.

## Estrategia de pruebas

- **Compilación:** `-Wall -Werror` del `.vpk` con VitaSDK, en local y en el CI.
- **Tests en PC** (`make -C tests`, con ASan y UBSan):
  - `test_playlist`:
    - ida y vuelta de serializar y parsear con tildes y emoji;
    - BOM, CRLF, líneas vacías y `#` desconocidas;
    - archivo vacío, sin `#EXTM3U` o truncado (inválido, sin fugas);
    - añadir con duplicados y conteo de omitidas;
    - techo de 4000 con añadido parcial;
    - quitar en los extremos, mover hacia arriba, hacia abajo y a los extremos;
    - nombre vacío o de solo espacios, de 64 bytes y más, e igualdad plegada ("Favoritas" contra "FAVORÍTAS");
    - saneado de nombre de archivo y sufijos de colisión;
    - elección de álbumes del mosaico (A,A,B,C,B,D → A,B,C,D; saltar sin carátula; 0 a 3 álbumes);
    - tabla de recuperación tras un corte;
    - camino de error de memoria en `Parse` y `Append`, inyectando un fallo de reserva.
  - `test_cover_mosaic`: la reducción 2× de un patrón conocido y la ubicación de cada cuadrante.
  - `test_path_index`: ordenar y buscar rutas, con rutas repetidas y rutas ausentes.
- **Consola:** los escenarios de las specs, paso a paso, en las tareas. Además se anota el HEAP del overlay de debug antes y después de repetir 20 veces seguidas cada una de estas acciones:
  - entrar y salir de una playlist de 500 canciones;
  - añadir un álbum a una playlist;
  - recorrer la pestaña Playlists con 10 playlists con mosaico.
- **Datos de prueba:** las carpetas de prueba que ya existen en la tarjeta, más una playlist grande generada en el PC con el formato propio para medir la pestaña.

## CI

No requiere cambios en los workflows. `tests/Makefile` suma los binarios nuevos a `BIN` y el job existente ya corre `make -C tests`. El `.vpk` suma los fuentes nuevos en `CMakeLists.txt`.

## Terceros

No se incorpora código ni assets de terceros. M3U es un formato de texto sin licencia que lo restrinja.
