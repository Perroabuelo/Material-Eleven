## Why

Con `add-playlists`, añadir canciones a una playlist es de una en una, o un artista o un álbum entero. Armar una playlist con veinte canciones sueltas de la vista Canciones cuesta veinte veces el mismo recorrido por el menú, y limpiar una playlist es quitar canción por canción. Este cambio agrega la selección múltiple en las listas de la biblioteca, para añadir o quitar muchas canciones con una sola acción.

Rama: `change/multi-select`

Se apoya en `add-playlists` (v4.0.0, ya integrado en `main`): en su menú de Cuadrado contextual, en su selector de playlists y en la acción de quitar.

## What Changes

- **Entrar en el modo selección**, de dos formas:
  - desde el menú de Cuadrado, con la opción **Seleccionar**;
  - manteniendo el dedo sobre una fila, que además la deja marcada.
- **Dónde se puede seleccionar:** en las listas de canciones (Canciones, Añadidos recientemente y dentro de un artista o de un álbum), en las vistas Artistas y Álbumes, y dentro de una playlist. La pestaña Playlists no tiene modo selección.
- **Dentro del modo:**
  - Confirmar, o tocar una fila, marca o desmarca esa fila. Mantener el dedo sobre una fila hace lo mismo.
  - Cada fila muestra una casilla.
  - La barra superior se convierte en un **encabezado fijo** con un botón para salir, el contador de filas marcadas, la caja de búsqueda y una casilla **Todo** de tres estados (ninguna, algunas, todas) que se calcula sobre las filas visibles.
  - El encabezado entra en el recorrido de arriba y abajo: desde la primera fila, arriba lleva a la casilla Todo, y desde la casilla, arriba lleva a la última fila.
  - Triángulo sigue buscando. El botón de volver quita primero la búsqueda, y sin búsqueda sale del modo y descarta lo marcado.
  - L, R y SELECT no hacen nada mientras dura el modo. La leyenda cambia a las acciones del modo.
- **La casilla Todo actúa solo sobre lo visible.** Si todas las filas visibles están marcadas, las desmarca; si no, las marca todas. Las marcas que la búsqueda oculta no se tocan.
- **Cuadrado en modo selección** abre un menú con la acción sobre lo marcado:
  - Fuera de una playlist: **Añadir a playlist…**, con el mismo selector de `add-playlists`, incluida "Nueva playlist…".
  - Dentro de una playlist: **Quitar de la playlist**, que pide confirmación con cancelar marcado al abrirse.
  - Sin nada marcado, la acción aparece deshabilitada.
- **Lo marcado pertenece a la lista en que se marcó.** Cambiar el término de búsqueda conserva las marcas, también las de filas que el filtro oculta, y las acciones actúan sobre todas ellas. Salir del modo, cambiar de pantalla con el nav rail o terminar una acción descarta las marcas.
- **El orden de lo añadido es el de la lista completa**, sin filtro, no el orden en que se marcaron las filas. Varios álbumes o artistas aportan sus pistas grupo por grupo, cada uno en su orden interno. Las canciones que ya estaban en la playlist se omiten, y el aviso lo dice, igual que en `add-playlists`.
- **El reconocedor táctil gana el toque mantenido.** Es lógica pura, con su test en PC.

## Capabilities

### New Capabilities

- `library/multi-select`: el modo selección de las listas de la biblioteca. Define dónde está disponible, cómo se entra y se sale, el encabezado con la casilla Todo, qué hacen los botones y el toque, cómo se relaciona con la búsqueda y su leyenda.

### Modified Capabilities

- `library/views`: el menú de Cuadrado suma "Seleccionar", y en modo selección ofrece solo la acción sobre lo marcado.
- `library/playlists`: añadir acepta lo marcado (varias canciones, álbumes o artistas) en una sola acción, con un único aviso. Quitar acepta varias canciones a la vez, con confirmación.
- `ui/list-navigation`: mantener el dedo sobre una fila de la biblioteca es un gesto propio, distinto de tocar y de arrastrar. En modo selección, tocar una fila la marca en vez de activarla, y el recorrido de arriba y abajo incluye el encabezado.

## Impact

- `source/gesture.c` e `include/gesture.h`: evento de toque mantenido, con su umbral de tiempo, y su test en `tests/test_gesture.c`.
- `source/touch_list.c`: entrega el toque mantenido a la pantalla que lo pide. Carpetas y Ajustes no lo piden y no cambian.
- Módulo puro nuevo con el conjunto de marcas sobre posiciones de la lista completa: marcar, desmarcar, estado de la casilla Todo sobre lo visible, contar y recorrer en orden. Lleva tests en PC.
- `source/playlist.c`: `Playlist_Append` ya recibe varias entradas. Se agrega quitar varias de una vez, con su test.
- `source/menus/menu_library.c`:
  - estado del modo y casillas en las filas;
  - encabezado con contador y casilla Todo, dentro del recorrido de arriba y abajo;
  - menú de Cuadrado del modo y confirmación de quitar varias;
  - leyenda nueva, y L, R y SELECT bloqueados.
- `include/lang_strings.h`: textos del modo, del contador, de la confirmación y de los avisos.
- No se agregan dependencias de terceros.

## Fuera de alcance

- Una "canasta" que conserve lo marcado al cambiar de pestaña, al entrar en un grupo o al salir de él.
- Selección múltiple en la pestaña Playlists, por ejemplo para borrar varias playlists.
- Selección múltiple en Carpetas.
- Seleccionar un rango entre dos filas manteniendo un botón.
- Mover varias canciones a la vez dentro de una playlist.
- Otras acciones sobre lo marcado: reproducir solo lo marcado o ponerlo en la cola.
- Las acciones de playlist desde Reproduciendo.

## Notas de version

Versión objetivo: **v4.1.0** (minor: funcionalidad nueva), la siguiente a v4.0.0.

- Select several songs at once: choose "Select" from the Square menu, or touch and hold a row.
- Add all selected songs, albums or artists to a playlist in one go.
- Remove several songs from a playlist at once.
- The "All" checkbox at the top selects every row in view, so you can search and then add all the results. Search again to keep adding to the same selection.
