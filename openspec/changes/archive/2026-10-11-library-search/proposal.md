## Why

La biblioteca no tiene forma de buscar: para llegar a una canción entre cientos o miles hay que recorrer la lista fila por fila. El navegador de carpetas sí tiene un filtro, pero solo se abre tocando la pantalla, no encuentra "canción" si se escribe "cancion" y, al convertir lo que devuelve el teclado, destroza cualquier letra acentuada que se escriba. Este cambio lleva la búsqueda a la biblioteca y, con el mismo código, corrige la de carpetas.

Rama: `change/library-search`

## What Changes

- La biblioteca gana una búsqueda sobre la lista que se ve. Se abre con Triángulo o tocando una caja de búsqueda en la barra superior, igual que en Carpetas.
  - En Canciones y Añadidos recientemente compara el título, el artista y el álbum de cada pista.
  - En Artistas y Álbumes compara el nombre del grupo.
  - Dentro de un artista o de un álbum compara el título de la pista.
- El término pertenece a la lista en que se escribió. Cambiar de pestaña con L/R lo borra. Ir a otra pantalla con el nav rail y volver lo conserva, y volver desde Reproduciendo también.
- Círculo deshace lo más reciente primero: quita el término de la lista que se ve y solo después sale del grupo. Cuando la búsqueda no deja resultados, la pantalla lo dice en vez de mostrar una lista vacía.
- Reproducir desde una lista filtrada **no** convierte el resultado en la cola. La cola es la vista completa, sin filtrar, y empieza en la pista elegida, como si se hubiera elegido sin buscar. Buscar una canción concreta no deja una cola de una sola pista.
- **BREAKING (botones)**: en la Biblioteca, Triángulo deja de reescanear y pasa a buscar. Cuadrado abre un menú pequeño con "Reescanear" y "Elegir carpeta", así que las dos acciones siguen estando en la Biblioteca. Reescanear quita la búsqueda.
- Carpetas gana Triángulo para abrir su filtro, que hoy solo se abre tocando la pantalla.
- La comparación, en Carpetas y en Biblioteca, no distingue mayúsculas ni acentos ("cancion" encuentra "CANCIÓN"). Lo que se escribe con acentos llega intacto al filtro.
- El diálogo de texto del sistema y sus salidas de emergencia (L+R+START, abandono si no compone, tope de tiempo) salen de `menu_displayfiles.c` a un módulo compartido que usan las dos pantallas.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `library/views`: se agrega la búsqueda en la biblioteca y su salida. Reproducir desde una lista filtrada usa como cola la vista sin filtrar. Reescanear y elegir carpeta se piden desde el menú de Cuadrado, y reescanear quita la búsqueda.
- `ui/folder-browser`: el filtro también se abre con Triángulo, compara sin distinguir mayúsculas ni acentos, y conserva el texto acentuado que se escribe.
- `ui/nav-shell`: al volver a la Biblioteca desde Reproduciendo también se conserva la búsqueda. La leyenda muestra Triángulo junto a "Buscar" en Biblioteca y en Carpetas.

## Impact

- `source/menus/menu_displayfiles.c`: pierde el lazo del diálogo y lo llama desde el módulo nuevo. Triángulo abre el filtro. La leyenda suma la entrada de buscar.
- `source/menus/menu_library.c`: término por lista, caja de búsqueda en la barra superior, menú de Cuadrado, traducción del índice filtrado al de la vista completa al reproducir y leyenda nueva.
- Módulo nuevo del diálogo de búsqueda (SCE e IME), compartido por las dos pantallas.
- Módulos puros nuevos, con tests en PC: comparación sin mayúsculas ni acentos sobre UTF-8, conversión UTF-8 a UTF-16 y de vuelta para el diálogo, y la lista filtrada (índices visibles y traducción al índice completo).
- `source/dirbrowse.c`: `Dirbrowse_NameContains` pasa a usar la comparación compartida.
- `include/lang_strings.h`: textos nuevos (buscar en la biblioteca, sin resultados, el menú de Cuadrado y sus dos opciones, y la leyenda "Opciones" de Cuadrado).
- `tests/Makefile`: binarios de test nuevos. El CI ya corre `make -C tests`, así que no requiere cambios.
- No se agregan dependencias de terceros.

## Fuera de alcance

- Buscar fuera de la carpeta actual en el navegador de carpetas. Sigue sin haber índice ni búsqueda entre carpetas.
- Una búsqueda global que filtre todas las pestañas a la vez, o que muestre resultados mezclados de canciones, artistas y álbumes.
- Buscar por otros campos (género, año, ruta) o con operadores.
- Búsqueda incremental mientras se escribe. El filtro se aplica al confirmar el diálogo del sistema.
- Historial de búsquedas, o conservarlas entre arranques de la aplicación.
- La navegación táctil de las listas (scroll y toque sobre filas). Va en `change/touch-navigation`.
- La repetición de teclas al mantener presionado. Va en `change/key-repeat`.

## Notas de version

Versión objetivo: **v3.8.0** (minor: funcionalidad nueva). Supone que `key-repeat` sale antes como v3.7.0. Si el orden cambia, se ajusta al cerrar el cambio.

- Search your library: press Triangle or tap the search box to filter the current list by title, artist or album.
- Playing a song from search results keeps the whole list as the queue, starting at that song.
- Square in the Library now opens a menu with Rescan and Choose folder.
- Triangle now opens the filter in Folders too.
- Search ignores case and accents: "cancion" finds "Canción", in Folders and Library.
- Fixed accented letters typed into the folder filter being garbled.
