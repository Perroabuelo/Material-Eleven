## 1. Modelo puro de playlist

- [x] 1.1 Crear `source/playlist.c` e `include/playlist.h`, sin SCE ni vita2d, con el struct `Playlist`, `Playlist_Parse`, `Playlist_Serialize` y `Playlist_Free` según el formato de la Decisión 2. Crear `tests/test_playlist.c` con los casos de formato de la estrategia de pruebas de design.md:
  - ida y vuelta con tildes y emoji;
  - BOM, CRLF, líneas vacías y `#` desconocidas;
  - archivo vacío, sin `#EXTM3U` o truncado;
  - fallo de reserva inyectado en `Parse`, sin fugas.

  Agregar el test a `BIN` y a las reglas de `tests/Makefile`, y el fuente a `CMakeLists.txt`. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile sin warnings.
- [x] 1.2 Agregar `Playlist_Append` (sin duplicados, devuelve añadidas, omitidas y si se llenó, con techo 4000), `Playlist_Remove` y `Playlist_Move`. Ampliar `test_playlist`:
  - duplicados y conteo de omitidas;
  - 3998 + 5 deja 4000 y marca llena;
  - quitar primera, del medio y última;
  - mover hacia arriba, hacia abajo, a los extremos y fuera de rango;
  - fallo de reserva en `Append`, que deja la playlist como estaba.

  Listo cuando `make -C tests` pase con sanitizers.
- [x] 1.3 Agregar las reglas de nombre (Decisión 3): `Playlist_NameValid` (no vacío ni solo espacios, como mucho 64 caracteres), `Playlist_NamesEqual` con `TextFold_Fold`, y `Playlist_FileName` con saneado, recorte UTF-8 sin partir caracteres y sufijo ` (n)` ante colisión, recibiendo una función que dice si un nombre ya existe. Ampliar `test_playlist` con los casos de nombre de design.md, incluido `AC/DC: lo mejor?`. Listo cuando `make -C tests` pase con sanitizers.
- [x] 1.4 Agregar `Playlist_Recover` con la tabla de la Decisión 4, y `Playlist_MosaicAlbums`, que recibe una función de "¿tiene carátula?" (Decisión 8). Ampliar `test_playlist`:
  - las tres filas de la tabla, más "solo main" y "nada";
  - A,A,B,C,B,D da A,B,C,D;
  - saltar álbumes sin carátula;
  - 0, 1, 2 y 3 álbumes distintos;
  - álbum vacío con clave por ruta.

  Listo cuando `make -C tests` pase con sanitizers.

## 2. Almacenamiento

- [ ] 2.1 Crear `source/playlist_store.c` e `include/playlist_store.h`:
  - `PlaylistStore_List` aplica `Playlist_Recover` a cada terna y devuelve resúmenes (nombre, archivo, cantidad, mtime, álbumes del mosaico) ordenados por mtime descendente, saltando los archivos inválidos.
  - `PlaylistStore_Load` y `PlaylistStore_Save` con temporal, `.bak` y renombrado (Decisión 4).
  - `PlaylistStore_Rename` en dos pasos y `PlaylistStore_Delete`.

  Cada función cierra su fd y libera su buffer en un único punto de salida (tabla de recursos de design.md). Agregar el fuente a `CMakeLists.txt`. Listo cuando `scripts/build.sh` compile sin warnings. Se verifica en consola en el grupo 4.

## 3. Biblioteca y carátulas

- [ ] 3.1 Índice por ruta (Decisión 7): funciones puras de ordenar y buscar sobre un arreglo de rutas, con `tests/test_path_index.c` (rutas repetidas, ausentes, vacío y fallo de reserva). `library.c` construye el índice al final de `Library_Load` y del escaneo, lo libera en `Library_Free` y expone `Library_FindPath`. Listo cuando:
  - `make -C tests` pase con sanitizers;
  - `scripts/build.sh` compile;
  - y en la consola reescanear la biblioteca de prueba 10 veces deje el HEAP del overlay de debug igual que tras el primer escaneo, anotando antes y después.
- [ ] 3.2 Mosaico (Decisión 8): reducción 2× pura con `tests/test_cover_mosaic.c` (patrón conocido y ubicación de cada cuadrante), y `Cover_GetMosaic` en `cover.c` con clave por contenido, presupuesto compartido con `Cover_Get`, guardado solo cuando el mosaico es definitivo y borrado de la clave anterior. Listo cuando `make -C tests` pase y `scripts/build.sh` compile. Se verifica en consola en 5.3.

## 4. Pestaña Playlists y nivel interior

- [ ] 4.1 Agregar `VIEW_PLAYLISTS` y la etiqueta "Playlists" en los dos idiomas a `lang_strings.h`. La pestaña lista los resúmenes con nombre y cantidad, todavía con el marcador como carátula. Mostrar el estado vacío con su explicación. Liberar los resúmenes al salir de la pestaña y de la pantalla. Listo cuando:
  - `make -C tests` pase (`test_lang`);
  - `scripts/build.sh` compile;
  - en la consola, con dos `.m3u8` copiados a mano en `ux0:data/ElevenMPV/playlists/`, se cumplan "Llegar a las playlists" de `library/views` y "Sin playlists" de `library/playlists`;
  - las cinco pestañas, la caja de búsqueda y el título se vean sin solaparse en English y en Español (riesgo de design.md);
  - un `.m3u8` vacío y otro con basura no rompan la pestaña ("Un archivo dañado no rompe la pestaña");
  - y se mida el tiempo de entrada con una playlist de 4000 canciones generada en el PC, anotándolo.
- [ ] 4.2 Entrar en una playlist (Decisión 9: `inside_kind`, nombre de archivo de la playlist abierta), con sus filas como las de un álbum, las no disponibles atenuadas con el título y el artista guardados, y "playlist vacía". Reproducir con la Decisión 10. Liberar la playlist abierta al salir de ella, al cambiar de pestaña y al salir de la pantalla. Listo cuando `scripts/build.sh` compile y en la consola se cumplan:
  - "Entrar en una playlist" de `library/views`;
  - todos los escenarios de "Reproducir una playlist" y de "Canciones que la biblioteca ya no tiene" de `library/playlists`;
  - "Volver a la playlist de origen" de `ui/nav-shell`;
  - y el HEAP antes y después de entrar y salir 20 veces de una playlist de 500 canciones, anotándolo.
- [ ] 4.3 Búsqueda en la pestaña y dentro de una playlist, con las comparaciones de `library/views`. Listo cuando `scripts/build.sh` compile y en la consola se cumplan "Buscar una playlist", "Buscar dentro de una playlist" y "Reproducir un resultado dentro de una playlist".

## 5. Crear y añadir

- [ ] 5.1 Convertir el menú de Cuadrado en una lista armada al abrirlo según la fila marcada (Decisión 9), por ahora con "Añadir a playlist…" sobre canciones y sobre artistas y álbumes, más reescanear y elegir carpeta. Convertir el aviso en texto formateado con `Menu_LibraryNoticef`. Agregar los textos a `lang_strings.h`. Listo cuando:
  - `make -C tests` pase;
  - `scripts/build.sh` compile;
  - en la consola se cumplan "El menú sobre una canción" y "El menú en la pestaña Playlists vacía" de `library/views`;
  - y los escenarios previos del menú sigan pasando: "Pedir un reescaneo", "Cerrar el menú sin hacer nada" y "Reescanear quita la búsqueda".
- [ ] 5.2 Selector de playlists con "Nueva playlist…" (`SearchDialog_Prompt`), crear y añadir, añadir a una existente, y avisos de añadidas, omitidas, llena y nombre repetido. La playlist destino se carga, se modifica, se guarda y se libera en cada acción. Listo cuando `scripts/build.sh` compile y en la consola se cumplan todos los escenarios de:
  - "Crear una playlist con nombre";
  - "Añadir canciones a una playlist" (el de playlist llena ya lo cubre `test_playlist`);
  - y "Una playlist sobrevive al reinicio" y "El archivo es un M3U legible" de `library/playlists`.

  Anotar el HEAP antes y después de añadir un álbum a una playlist 20 veces.
- [ ] 5.3 Mostrar el mosaico en las filas de la pestaña con `Cover_GetMosaic`. Listo cuando `scripts/build.sh` compile y en la consola se cumplan todos los escenarios de "Presentación de las playlists" de `library/playlists`, incluido "Recorrer la pestaña con muchas playlists" con el HEAP anotado antes y después de recorrerla 20 veces.

## 6. Editar

- [ ] 6.1 Dentro de una playlist, menú con "Quitar de la playlist" y "Mover", con el modo mover y su leyenda. Listo cuando `scripts/build.sh` compile y en la consola se cumplan todos los escenarios de "Quitar y mover canciones dentro de una playlist" de `library/playlists`.
- [ ] 6.2 En la pestaña, menú con "Renombrar" (teclado del sistema con el nombre cargado) y "Borrar" con el diálogo de confirmación, con "Cancelar" marcada al abrir. Listo cuando `scripts/build.sh` compile y en la consola se cumplan:
  - todos los escenarios de "Renombrar y borrar una playlist" de `library/playlists`;
  - "El menú sobre una playlist" de `library/views`;
  - y "Volver a una playlist que se borró" de `ui/nav-shell`.
- [ ] 6.3 Escenario de corte en consola: con una playlist de 500 canciones abierta, quitar una canción y apagar la consola manteniendo el botón de encendido justo después. Listo cuando, al reiniciar, la playlist aparezca entera en su versión anterior o en la nueva, nunca vacía, y no queden `.tmp` ni `.bak` en `ux0:data/ElevenMPV/playlists/` después de abrir la pestaña. El escenario "Guardado interrumpido" ya lo cubre `test_playlist` (1.4).

## 7. Versión y cierre

- [ ] 7.1 Agregar los bullets de Notas de versión de `proposal.md` a `CHANGELOG.md` bajo `[Unreleased]` en `### Added`, y subir `VITA_VERSION` en `CMakeLists.txt` a la versión objetivo de la propuesta, en el formato que exige `.github/scripts/check-version.sh`. Listo cuando `scripts/build.sh --clean` compile sin warnings y el `.vpk` instalado encima de la versión anterior muestre la nueva y conserve la configuración, el índice y las playlists.
- [ ] 7.2 Push de `change/add-playlists` y PR a `main`. Listo cuando el CI del PR esté en verde: build del `.vpk` y tests en PC, incluidos `test_playlist`, `test_path_index` y `test_cover_mosaic`.
