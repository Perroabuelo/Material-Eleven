## Why

Hoy la única forma de escuchar una selección propia de canciones es tenerlas en la misma carpeta o en el mismo álbum. La cola ya no depende de la carpeta y acepta cualquier lista de pistas, así que falta lo que el usuario sí ve: poder juntar canciones de cualquier parte de su biblioteca en una lista con nombre, guardarla y volver a reproducirla. Este cambio agrega las playlists, creadas y editadas en la propia consola.

Rama: `change/add-playlists`

## What Changes

- La biblioteca gana una quinta pestaña, **Playlists**, después de Añadidos recientemente. Muestra las playlists del usuario, de la modificada más recientemente a la más antigua, cada una con su nombre, su cantidad de canciones y su carátula.
- La carátula de una playlist es un mosaico con las carátulas de los primeros cuatro álbumes distintos que aparecen en ella. Con uno a tres álbumes distintos muestra una sola carátula, la del primero, y sin ninguna muestra el marcador por defecto.
- El menú de Cuadrado de la biblioteca pasa a depender de la fila marcada:
  - Sobre una canción ofrece **Añadir a playlist…**.
  - Sobre un artista o un álbum ofrece añadir todas sus pistas, en el orden que muestra ese grupo.
  - Siguen estando Reescanear y Elegir carpeta.
- **Añadir a playlist…** abre un selector con **Nueva playlist…** primero y después las playlists, la usada más recientemente arriba. "Nueva playlist…" pide el nombre con el teclado del sistema, crea la playlist y añade en un solo paso. Las canciones que ya estaban en la playlist no se repiten, y un aviso breve dice cuántas se añadieron y cuántas ya estaban.
- Dentro de una playlist:
  - Confirmar sobre una canción la reproduce, y la cola pasa a ser la playlist completa desde esa canción. El ajuste "Al terminar un álbum o artista" no la afecta.
  - Cuadrado sobre una canción ofrece **Quitar de la playlist** y **Mover**. Mover entra en un modo en que arriba y abajo desplazan la canción, y confirmar la deja en su nuevo lugar.
  - Las canciones cuyo archivo ya no existe se muestran atenuadas y se saltan al reproducir.
- Cuadrado sobre una playlist, en la pestaña Playlists, ofrece **Renombrar** y **Borrar**. Borrar pide confirmación.
- La cola que suena es una copia de la playlist en el momento de reproducirla. Editar la playlist que suena no cambia lo que sigue sonando: los cambios rigen la próxima vez que se reproduzca.
- La búsqueda de la biblioteca funciona también aquí: en la pestaña Playlists compara el nombre de la playlist, y dentro de una playlist compara el título de la canción.
- Desde Reproduciendo, el botón de volver lleva a la playlist desde la que se eligió la canción, con la misma fila seleccionada.
- Las playlists se guardan como archivos `.m3u8` en `ux0:data/ElevenMPV/playlists/`. Se guardan de forma que un corte a mitad de escritura no deje una playlist corrupta.

## Capabilities

### New Capabilities

- `library/playlists`: qué es una playlist, dónde y cómo se guarda, cómo se crea, se renombra y se borra, cómo se añaden, se quitan y se mueven canciones, qué ocurre con las canciones que ya no existen, cómo se reproduce y de qué se compone su carátula.

### Modified Capabilities

- `library/views`: la biblioteca pasa de cuatro a cinco vistas con la pestaña Playlists. El menú de Cuadrado depende de la fila marcada y suma las acciones de playlist. La búsqueda define qué se compara en la pestaña Playlists y dentro de una playlist. Reproducir desde una playlist usa la playlist como cola.
- `ui/nav-shell`: el botón de volver de Reproduciendo también regresa a la playlist de origen, con la misma fila seleccionada.

## Impact

- `source/menus/menu_library.c`:
  - Pestaña nueva, con el nivel "dentro de una playlist".
  - Menú de Cuadrado contextual, selector de playlists, modo mover y diálogo de confirmación.
  - Leyenda nueva para cada estado.
- Módulo puro nuevo de playlists, con tests en PC. Lee y escribe `.m3u8` con `#EXTM3U`, `#PLAYLIST`, `#EXTINF` y `#EXTALB`. Opera sobre la lista: añadir sin duplicados, quitar, mover, renombrar, sanear el nombre de archivo y respetar el techo de pistas.
- Módulo de almacenamiento de playlists sobre el sistema de archivos. Lista la carpeta, carga, guarda escribiendo a un temporal y renombrando, y borra.
- `source/cover.c`: composición del mosaico de cuatro carátulas en una textura y su caché en disco. La reducción y la copia de píxeles se escriben como lógica pura, con test en PC.
- `include/gesture.h` no cambia: el toque mantenido llega en `multi-select`.
- `source/queue.c` no cambia de contrato. La playlist se vuelca con `Queue_Add`, igual que una vista.
- `include/lang_strings.h`: textos nuevos (pestaña, menú contextual, selector, nombre de la playlist, avisos, confirmación de borrado, playlists vacías y leyendas).
- `tests/Makefile`: binarios de test nuevos. El CI ya corre `make -C tests`.
- No se agregan dependencias de terceros.

## Fuera de alcance

- Selección múltiple y toque mantenido sobre las filas. Se entregan en el cambio `multi-select` (v3.11.0), que se apoya en este.
- Acciones de playlist desde Reproduciendo, como "Añadir la que suena" o "Guardar cola como playlist". Siguen vigentes los requirements de `ui/now-playing` que no ofrecen gestionar la cola.
- Añadir a una playlist desde Carpetas. Las playlists pertenecen a la biblioteca.
- Importar o reconocer playlists hechas en el PC, incluidos los `.m3u` que estén en las carpetas de música.
- Actualizar la cola en vivo cuando se edita la playlist que está sonando.
- Ordenar la pestaña Playlists de otra forma que por última modificación.
- Playlists inteligentes o automáticas (más escuchadas, favoritas por puntuación).
- El visualizador de audio.

## Notas de version

Versión objetivo: **v3.10.0** (minor: funcionalidad nueva). Supone que no sale otra versión antes. Si el orden cambia, se ajusta al cerrar el cambio.

- New Playlists tab in the Library: make your own playlists right on the Vita.
- Press Square on a song, an album or an artist and choose "Add to playlist" to add it to an existing playlist or to a new one.
- Inside a playlist, press Square on a song to remove it or move it to another position.
- Rename or delete a playlist with Square in the Playlists tab.
- Each playlist shows a cover made from the first four albums in it.
- Songs whose file is gone are shown dimmed and skipped during playback.
