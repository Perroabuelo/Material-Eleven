## 1. Números de pista y de disco

- [x] 1.1 Crear el módulo puro `source/track_meta.c` / `include/track_meta.h` con `TrackMeta_ParseNumber`, `TrackMeta_Id3v1Track`, `TrackMeta_CompareDisc` y `TrackMeta_CompareGrouped` (decisión 1 de design.md), sin vita2d, SCE ni `psp2/types.h`. Agregar `tests/test_track_meta.c` a `tests/Makefile` con los casos de la estrategia de pruebas, y el módulo a `CMakeLists.txt`. Listo cuando `make -C tests` pase (con `test_lang`, `test_screen_off` y `test_accent`) y `scripts/build.sh` compile con `-Wall -Werror`.
- [x] 1.2 Sumar `track` y `disc` a `Tags` y llenarlos en `tags.c` desde los comentarios Vorbis, Opus, los frames ID3v2 `TRCK`/`TPOS` y el ID3v1.1 (decisión 2). Antes de escribir, confirmar que el `mpg123.h` del VitaSDK expone `text`/`texts` en `mpg123_id3v2`, y anotar acá el resultado. Listo cuando compile con `-Wall -Werror`.
  - Resultado: el `mpg123.h` del VitaSDK expone `mpg123_text *text` y `size_t texts` en `mpg123_id3v2`, y cada `mpg123_text` trae `char id[4]` (sin terminador) y `mpg123_string text`. MP3 lee `TRCK` y `TPOS` de ahí, y ID3v1.1 queda como respaldo del número de pista.

## 2. Índice v3

- [x] 2.1 Crear el módulo puro `source/library_row.c` / `include/library_row.h` con `LibraryRow_GetLayout` para las versiones 2 y 3 (decisión 3). Agregar `tests/test_library_row.c` a `tests/Makefile` (v2, v3 y una versión desconocida) y el módulo a `CMakeLists.txt`. Listo cuando `make -C tests` pase y el `.vpk` compile con `-Wall -Werror`.
- [ ] 2.2 Sumar `track` y `disc` a `Library_Track`. Hacer que `Library_Save` escriba v3, que `Library_Load` acepte v2 (con `tagged = false` y sin números) y v3 usando `LibraryRow_GetLayout`, y que la pasada de tags guarde `track` y `disc`. Cambiar `Library_CarryTags` para que copie título, artista, álbum, números y el valor de `tagged` siempre que el archivo no haya cambiado (decisión 4). Listo cuando compile con `-Wall -Werror` y, en consola, con un índice v2 de la versión anterior: la Biblioteca se muestre completa con el contador de tags pendientes igual al total; un reescaneo abandonado al empezar la pasada de tags deje las vistas Artistas y Álbumes con sus nombres; y cerrar y reabrir la app conserve los números ya leídos.

## 3. Orden y reproducción continua

- [ ] 3.1 Hacer que `Library_BuildFieldTracks` ordene con `TrackMeta_CompareDisc` dentro de un álbum y con `TrackMeta_CompareGrouped` (grupo = álbum) dentro de un artista, y agregar `Library_BuildContinuous` (decisiones 5 y 6). Listo cuando compile con `-Wall -Werror` y, en consola, después de releer tags: un álbum con títulos fuera de orden alfabético se vea del 1 al N; un álbum de dos discos muestre primero el disco 1; un artista con dos álbumes y pistas sin álbum se vea agrupado y con las sin álbum al final; y la vista Canciones siga ordenada por título.
- [ ] 3.2 Pasar `config.cfg` a v4 con `group_end` y agregar la categoría "Reproducción" en Ajustes con sus textos en los dos idiomas (decisión 9). Hacer que `Menu_LibraryPlaySelected` arme la cola continua según el ajuste, en el orden de pasos de la decisión 6. Listo cuando compile con `-Wall -Werror`, `make -C tests` pase y, en consola:
  1. Un `config.cfg` v3 con idioma English y ecualizador Pop conserve los dos y el ajuste nuevo quede en "Repeat it".
  2. Con "Repetirlo", la última pista de un álbum siga con la primera del mismo álbum.
  3. Con "Seguir con el siguiente", la última pista de un álbum siga con la primera del álbum que sigue en la vista Álbumes, y "A continuación" lo anuncie antes.
  4. Lo mismo dentro de un artista, que siga con el artista que sigue en la vista Artistas.
  5. El último álbum de la vista Álbumes siga con el primero.
  6. Con el barajado encendido, la reproducción salga de toda la biblioteca.
  7. Desde la vista Canciones, el ajuste no cambie nada.
  8. Cambiar el ajuste mientras suena un álbum no cambie "A continuación".
  9. Al volver a la Biblioteca desde Reproduciendo, se vea la lista del grupo y no la biblioteca entera.
  10. La categoría nueva quepa en la lista sin tapar la barra de botones.

## 4. Carátulas nuevas

- [ ] 4.1 Hacer que `Queue_Add` reciba el álbum y que `Queue_PeekAhead` lo devuelva: la Biblioteca pasa `track->album` y Carpetas pasa `NULL`. Dibujar la carátula en "A continuación" con un presupuesto de 1 lectura por fotograma (decisión 7). Listo cuando compile con `-Wall -Werror` y, en consola: desde un álbum con carátula, las dos próximas pistas la muestren; con "Seguir con el siguiente" y la penúltima pista sonando, se vean la carátula del álbum actual y la del siguiente; desde Carpetas, se vea el marcador; y cambiar de pista no muestre tirones.
- [ ] 4.2 Agregar `Cover_Probe` a `cover.c`. Armar en `Library_BuildFieldNames(ARTIST)` los candidatos de cada artista, y resolverlos al dibujar las filas visibles de la vista Artistas, dentro del presupuesto por fotograma (decisión 8). Listo cuando compile con `-Wall -Werror` y, en consola: un artista con algún álbum con carátula la muestre, siempre la misma; un artista sin carátulas y el cubo "Desconocido" muestren el marcador; y recorrer rápido una lista de artistas larga no se detenga.

## 5. Verificación de la actualización completa

- [ ] 5.1 En consola, instalar el `.vpk` sobre una v3.3.x que tenga ajustes cambiados y la biblioteca escaneada con carátulas. Verificar que los ajustes se conservan, que la Biblioteca se ve completa al primer arranque sin escanear sola, que el contador muestra todas las pistas pendientes, y que, después de reescanear con △ y dejar terminar la pasada de tags, los álbumes siguen el orden del disco. Listo cuando todo lo anterior se cumpla y el resultado, con el tiempo que tardó la pasada de tags y el número de pistas, quede anotado en esta tarea.
