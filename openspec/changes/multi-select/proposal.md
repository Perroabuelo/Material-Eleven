## Why

Con `add-playlists`, añadir canciones a una playlist es de una en una, o un artista o un álbum entero. Armar una playlist con veinte canciones sueltas de la vista Canciones cuesta veinte veces el mismo recorrido por el menú, y limpiar una playlist es quitar canción por canción. Este cambio agrega la selección múltiple en las listas de la biblioteca, para añadir o quitar muchas canciones con una sola acción.

Rama: `change/multi-select`

Depende de `add-playlists`: se apoya en su menú de Cuadrado contextual, en su selector de playlists y en la acción de quitar. Su rama se crea desde `main` una vez integrado `add-playlists`.

## What Changes

- **Entrar en el modo selección**, de dos formas:
  - desde el menú de Cuadrado, con la opción **Seleccionar**;
  - manteniendo el dedo sobre una fila, que además la deja marcada.
- **Dentro del modo:**
  - Confirmar, o tocar una fila, marca o desmarca esa fila.
  - Cada fila muestra una casilla y la barra superior muestra cuántas hay marcadas, con un botón para salir.
  - El botón de volver sale del modo y descarta lo marcado.
  - La leyenda cambia a las acciones del modo.
- **Cuadrado en modo selección** abre un menú de acciones sobre lo marcado:
  - Fuera de una playlist: **Añadir N a playlist…**, que usa el mismo selector de `add-playlists`, incluida "Nueva playlist…".
  - Dentro de una playlist: **Quitar N de la playlist**.
  - En los dos casos: **Seleccionar todo** y **Ninguno**.
- **Seleccionar todo marca solo lo que se ve.** Combinado con la búsqueda, filtrar y seleccionar todo añade exactamente los resultados.
- **En las vistas Artistas y Álbumes también se pueden marcar filas.** Añadir varios álbumes o artistas añade sus pistas, grupo por grupo, en el orden en que se muestran en la lista y cada grupo en su orden interno.
- **Lo marcado pertenece a la lista en que se marcó.** Cambiar el término de búsqueda conserva las marcas, incluso las de filas que el filtro oculta. En cambio, salir del modo, cambiar de pantalla con el nav rail, entrar en un grupo o salir de él descarta las marcas. L y R no cambian de pestaña mientras dura el modo.
- **El orden de lo añadido es el de la lista**, no el orden en que se marcaron las filas. Las canciones que ya estaban en la playlist se omiten, y el aviso lo dice, igual que en `add-playlists`.
- **El reconocedor táctil gana el toque mantenido.** Es lógica pura, con su test en PC.

## Capabilities

### New Capabilities

- `library/multi-select`: el modo selección de las listas de la biblioteca, cómo se entra y se sale, qué se puede marcar, cómo se relaciona con la búsqueda y las acciones disponibles sobre lo marcado.

### Modified Capabilities

- `library/views`: el menú de Cuadrado suma "Seleccionar". L y R no cambian de pestaña durante el modo selección.
- `library/playlists`: añadir y quitar aceptan varias canciones en una sola acción, con un único aviso.
- `ui/list-navigation`: mantener el dedo sobre una fila de la biblioteca es un gesto propio, distinto de tocar y de arrastrar.
- `ui/nav-shell`: la leyenda del modo selección.

## Impact

- `source/gesture.c` e `include/gesture.h`: evento de toque mantenido, con su umbral de tiempo, y su test en `tests/test_gesture.c`.
- `source/touch_list.c`: entrega el toque mantenido a la pantalla.
- `source/menus/menu_library.c`:
  - estado del modo y casillas en las filas;
  - barra superior con el contador y menú de Cuadrado del modo;
  - leyenda nueva y bloqueo de L y R.
- Módulo puro nuevo para el conjunto de marcas sobre índices de la vista completa: marcar, desmarcar, marcar todos los visibles, contar y recorrer en orden. Lleva tests en PC.
- `source/playlist.c`: sin cambios de contrato. `Playlist_Append` ya recibe varias entradas. Se agrega quitar varias de una vez, con su test.
- `include/lang_strings.h`: textos del modo, del contador y de las acciones.
- No se agregan dependencias de terceros.

## Fuera de alcance

- Una "canasta" que conserve lo marcado al cambiar de pestaña, al entrar en un grupo o al salir de él.
- Selección múltiple en Carpetas.
- Seleccionar un rango entre dos filas manteniendo un botón.
- Mover varias canciones a la vez dentro de una playlist.
- Otras acciones sobre lo marcado: reproducir solo lo marcado o ponerlo en la cola.
- Las acciones de playlist desde Reproduciendo.

## Notas de version

Versión objetivo: **v4.1.0** (minor: funcionalidad nueva), la siguiente a `add-playlists` (v4.0.0). Supone que no sale otra versión entre medio. Se ajusta al cerrar el cambio.

- Select several songs at once: choose "Select" from the Square menu, or touch and hold a row.
- Add all selected songs, albums or artists to a playlist in one go.
- Remove several songs from a playlist at once.
- "Select all" picks every row in view, so you can search and then add all the results.
