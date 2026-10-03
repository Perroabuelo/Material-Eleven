Rama: `change/album-order-and-continuous-play`, creada desde `main`. Su PR apunta a `main`.

## Why

La Biblioteca reproduce los álbumes en un orden que no es el del disco, y al terminar un álbum vuelve a empezarlo en vez de seguir con otro:

- **Las pistas de un álbum se ordenan por título, no por número de pista.** El escaneo no lee el número de pista ni el de disco, así que un álbum suena en orden alfabético ("Angel, Black Dog, Come Back..."). En Carpetas no se nota porque los archivos suelen llamarse `01 - ...`, `02 - ...`.
- **La reproducción da vueltas sobre el mismo álbum o artista.** La cola es la vista desde la que se reprodujo. Al terminar la última pista vuelve a la primera del mismo grupo, y no hay forma de que siga con el siguiente álbum o artista.
- **La vista Artistas no tiene imagen.** Cada fila muestra el marcador vacío, aunque sus álbumes tengan carátula.
- **"A continuación" (UP NEXT) en Reproduciendo muestra el marcador** en vez de la carátula de las próximas pistas, aunque la Biblioteca ya la tenga guardada.

## What Changes

- **Número de pista y de disco**: el escaneo lee el número de pista y el de disco de los tags (FLAC, OGG, OPUS y MP3) y los guarda en el índice.
- **Orden del disco dentro de un álbum**: disco, luego número de pista, luego título. Las pistas sin número van al final del álbum, por título.
- **Orden dentro de un artista**: por álbum, y dentro de cada álbum en el orden del disco. Las pistas sin álbum van al final.
- **El índice anterior se conserva al actualizar.** La biblioteca se ve completa desde el primer arranque, con sus pistas marcadas como pendientes de releer tags. Al reescanear (△), la segunda pasada lee los números de pista. Se puede abandonar y retomar, como hoy.
- **Ajuste nuevo "Al terminar un álbum o artista"**, con dos opciones:
  - *Repetirlo* (predeterminado): es el comportamiento de hoy.
  - *Seguir con el siguiente*: reproducir dentro de un álbum encadena todos los álbumes de la biblioteca, y dentro de un artista, todos los artistas. Con el barajado encendido se mezcla toda la biblioteca.

  Va en una categoría nueva, "Reproducción", de Ajustes.
- **Carátula en la vista Artistas**: cada artista muestra la carátula de la primera de sus pistas que tenga una. Sin ninguna, el marcador, igual que el cubo "Desconocido".
- **Carátula en "A continuación"**: las próximas pistas muestran su carátula cuando la cola viene de la Biblioteca. Con una cola de carpeta sigue el marcador.
- **Los ajustes se conservan al actualizar**: `config.cfg` pasa a v4, y un v3 conserva todos sus valores.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `library/index`: cada pista guarda también su número de pista y de disco, y un índice de la versión anterior se conserva al actualizar en vez de descartarse.
- `library/views`:
  - Las pistas de un álbum y de un artista siguen el orden del disco.
  - Reproducir dentro de un álbum o un artista puede encadenar los siguientes, según el ajuste nuevo.
  - La vista Artistas muestra carátula.
- `playback/queue`: el contenido que entrega la Biblioteca puede ser la biblioteca entera ordenada por grupo, no solo la vista abierta.
- `ui/now-playing`: "A continuación" muestra la carátula de las próximas pistas.
- `ui/settings`:
  - Categoría nueva "Reproducción" con el ajuste "Al terminar un álbum o artista".
  - Actualizar desde v3 conserva los ajustes.

## Impact

- **Código C**:
  - `source/tags.c` / `include/tags.h`: leer el número de pista y de disco (comentarios Vorbis `TRACKNUMBER` y `DISCNUMBER`, Opus, frames ID3v2 `TRCK` y `TPOS`, e ID3v1.1).
  - `source/library.c` / `include/library.h`: campos nuevos en `Library_Track`, índice v3 que también acepta v2, comparadores nuevos, vistas de biblioteca completa por álbum y por artista, y pista representativa de cada nombre.
  - `source/queue.c` / `include/queue.h`: la cola guarda el álbum de cada entrada, que la Biblioteca entrega y Carpetas no.
  - `source/menus/menu_library.c`: carátula de los artistas, y qué cola se arma según el ajuste.
  - `source/menus/menu_audioplayer.c`: carátula en "A continuación".
  - `source/menus/menu_settings.c`, `source/config.c` / `include/config.h`: categoría y ajuste nuevos, config v4.
  - `include/lang_strings.h`: textos nuevos en inglés y en español.
- **Pruebas en PC**: tests nuevos en `tests/` para leer números de pista, para los comparadores de orden y para leer filas del índice v2 y v3. Esa lógica sale a módulos puros nuevos, sin vita2d ni SCE.
- **Dependencias, assets y build**: ninguno nuevo.
- **CI**: sin cambios. El CI compila el `.vpk` y no corre `make -C tests`. Los módulos nuevos se agregan a `CMakeLists.txt`.

## Fuera de alcance

- Mostrar el número de pista en las filas de la Biblioteca.
- Ordenar la vista Canciones o Añadidos recientemente por algo distinto de lo que usan hoy.
- Una vista de discos separados dentro de un álbum.
- Carátula en "A continuación" para colas de carpeta, buscando cada archivo en el índice.
- Ordenar los álbumes de un artista por año (el índice no guarda el año).
- Iniciar sola la pasada de tags después de actualizar.
- Editar la cola o elegir a mano qué álbum sigue.

## Notas de version

Versión objetivo: **v3.4.0** (minor): agrega el orden por número de pista, la reproducción continua entre álbumes o artistas y nuevas carátulas.

- Albums in the Library now play in disc order, by disc and track number, instead of alphabetically by title.
- Inside an artist, songs are grouped by album and follow each album's track order.
- New setting under Settings > Playback: when an album or artist ends, repeat it or keep playing the next one.
- Artists in the Library now show a cover from one of their albums.
- Up Next on Now Playing shows the cover of each upcoming song when you play from the Library.
- Your existing library is kept after updating. Rescan with Triangle to read track numbers; you can stop and resume at any time.
