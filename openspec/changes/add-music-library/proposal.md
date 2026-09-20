## Why

Hoy la única forma de llegar a una canción es navegar carpetas: el usuario recorre `ux0:/`, `ur0:/` o `uma0:/` directorio por directorio, viendo por el camino todos los archivos que la aplicación no puede reproducir, y sin ninguna manera de ver su colección como colección. No existe nada parecido a una biblioteca: la cola de reproducción es literalmente la carpeta actual, reconstruida desde cero en cada `play` (`Menu_GetMusicList`, `source/menus/menu_audioplayer.c:34`), y desaparece al cambiar de directorio.

Se investigó si podía reutilizarse el escaneo de la aplicación nativa de la consola. No puede: su base de datos de metadatos no está expuesta por ninguna API pública, solo indexa `ux0:music` (contenido transferido por CMA) y no reconoce la mayoría de los formatos que son la razón de existir de este proyecto (FLAC, OPUS y los trackers XM/MOD/S3M/IT). Lo único aprovechable es `sceAppUtilMusicMount()`, que la aplicación ya llama en `source/utils.c:50` y hoy no usa para nada: monta `music0:` como sistema de archivos, sin metadatos. El escaneo tiene que ser propio.

## What Changes

- **Biblioteca como cuarta pantalla**, hermana de Now Playing, Folders y Settings en el nav rail. El navegador de carpetas **se conserva sin cambios funcionales**: son dos caminos en paralelo, no un reemplazo.
- **La biblioteca lleva el mismo mini reproductor que el navegador**, para que el transporte se alcance desde los dos caminos y no solo desde uno.
- **Una carpeta de escaneo**, elegida por el usuario y recordada entre sesiones, con reescaneo manual cuando añade música. Una sola carpeta, no una lista.
- **Índice persistente en disco** con ruta, extensión, tamaño, fecha de modificación, título, artista y álbum por pista.
- **Vistas de biblioteca**: Canciones, Artistas, Álbumes y Recientes (esta última derivada de la fecha de modificación, sin costo adicional de escaneo).
- **Las pistas sin artista o sin álbum caen en "Desconocido"**, que se ordena siempre al final de su lista. Afecta a los cinco formatos que no tienen tags —WAV y los cuatro de tracker— y a cualquier MP3 sin ID3.
- **Título de los módulos de tracker leído con `xmp_test_module`** (`libs/include/xmp.h:319`), que lee solo la cabecera y devuelve además el tipo de módulo. La ruta de reproducción usa `xmp_load_module` porque va a sonarlo (`source/audio/xm.c:20`); el escáner no debe pagar esa carga.
- **La cola de reproducción deja de derivarse de `cwd`** y pasa a recibir la lista desde donde el usuario pulsó play: una carpeta, una vista de biblioteca o, más adelante, una playlist.
- **Escáner en streaming**: recorre con `sceIoDread` entrada por entrada, sin reservar un arreglo de directorio de tamaño fijo. El techo de la biblioteca pasa a ser un presupuesto de memoria explícito y no el tamaño de un arreglo.
- **Se corrige una lectura de directorio sin límite** que el escaneo vuelve alcanzable: el bucle `while (sceIoDread(dir, &entries[entryCount]) > 0) entryCount++;` no compara nunca contra `MAX_FILES`, y escribe fuera de la reserva en una carpeta con más de 1024 entradas (`source/dirbrowse.c:69` y `source/menus/menu_audioplayer.c:41`). Hoy hay que provocarlo a mano; el escaneo recorre carpetas que el usuario nunca abre.
- **Entrega en tres fases, siempre en commits por partes.** Nunca un commit único: cada fase —y cada etapa dentro de una fase— se entrega en su propio commit, dejando el árbol compilable y la aplicación utilizable, para que un fallo sea atribuible por bisección.
  - **Fase 1** — recorrido, índice de rutas/tamaño/fecha, persistencia, reescaneo, pantalla de biblioteca con lista plana. Ya utilizable.
  - **Fase 2** — lectura de tags y vistas por artista y álbum. Aquí la biblioteca es lo que el usuario pidió.
  - **Fase 3** — carátulas, extraídas **una vez por álbum** y cacheadas como miniaturas en disco.
- **La fase 3 está condicionada a una medición previa.** Antes de comprometerla se mide sobre una colección real el costo de recorrer, de leer tags y de extraer y decodificar una carátula, y se determina si `vita2d_load_JPEG_buffer` usa el decodificador JPEG por hardware de la consola o una ruta por software. Si la medición la descarta, la fase 3 se retira del change —y sus requirements salen del spec— antes de archivarlo, en vez de quedar como deuda.

**Fuera de alcance, decidido explícitamente:** leer o escribir archivos `.m3u`/`.pls`; crear, nombrar o editar playlists en la consola (necesitan el teclado IME, formato de guardado y edición de cola, y modifican `ui/now-playing`); duración de las pistas en el índice; escanear más de una carpeta; y ocultar los archivos no reproducibles en el navegador de carpetas, que la biblioteca vuelve innecesario al dar un camino que no pasa por ahí.

## Capabilities

### New Capabilities
- `library/index`: de dónde sale la biblioteca y qué garantiza — elección y persistencia de la carpeta de escaneo, recorrido recursivo, qué campos lleva cada pista, cómo se resuelve la ausencia de tags, reescaneo, límites de tamaño y el comportamiento del escáner frente a carpetas grandes, archivos ilegibles y pistas desaparecidas.
- `library/views`: qué ve y qué puede hacer el usuario con ella — las vistas Canciones, Artistas, Álbumes y Recientes, el tratamiento de "Desconocido", el estado inicial sin carpeta elegida, el progreso del escaneo, y el hecho de que reproducir desde una vista convierte esa vista en la cola.

### Modified Capabilities
- `ui/nav-shell`: su primer requirement prohíbe hoy listar destinos inexistentes, y su escenario nombra "Library" como ejemplo de lo que el rail **no** debe mostrar. La biblioteca pasa a ser una destinación real y el rail pasa a listar cuatro.
- `ui/now-playing`: el requirement de vista previa de próximas pistas está redactado sobre "the current folder's playlist". Con la cola parametrizada, la cola puede venir de una vista de biblioteca, y la previsualización debe reflejar la cola vigente sea cual sea su origen. El requirement que prohíbe controles de gestión de cola **no cambia**: las playlists siguen fuera.
- `ui/folder-browser`: gana un requirement sobre el límite del listado, para que una carpeta con más entradas de las que la reserva admite se muestre truncada y estable en vez de corromper memoria. Ningún otro comportamiento del navegador cambia.

## Impact

- **Código nuevo**: un módulo de índice y escáner (`source/library.c`, `include/library.h`) y una pantalla de biblioteca (`source/menus/menu_library.c`, `include/menus/menu_library.h`).
- **Código modificado**: `source/menus/menu_audioplayer.c` (la cola deja de salir de `cwd`; `playlist[1024][512]` estático deja de ser la única fuente), `source/nav_rail.c` e `include/ui_theme.h` (cuarta destinación y su glifo, `UI_Screen`), `source/dirbrowse.c` y `source/menus/menu_audioplayer.c` (lectura de directorio acotada), `source/menus/menu_displayfiles.c` (el navegador en modo selección de carpeta, y el mini reproductor extraído para compartirlo), `CMakeLists.txt` (fuentes nuevas). La ruta de escaneo se persiste en su propio archivo, siguiendo el precedente de `lastdir.txt`, para no subir `CONFIG_VERSION` — que hoy descarta el config entero y borraría los ajustes del usuario (`source/config.c`).
- **Código reutilizado, no duplicado**: la extracción de carátulas ya existe para MP3 (`source/audio/mp3.c:156`), FLAC (`source/audio/flac.c:138`) y OPUS (`source/audio/opus.c:56`); OGG no la tiene (`source/audio/audio.c:150`). La fase 3 la aprovecha en vez de reescribirla.
- **Memoria**: el índice en RAM ronda 470 bytes por pista —~940 KB con 2000 pistas, ~1,9 MB con 4000— frente a los 512 KB que `playlist[1024][512]` ya ocupa hoy de forma estática y permanente. El escáner en streaming elimina el `calloc` de ~360 KB por carpeta que hace hoy cada navegación.
- **Disco**: el índice ocupa del orden de 500 KB con 2000 pistas. Las miniaturas de la fase 3, a 128x128 RGBA, ~64 KB por álbum.
- **Riesgo concentrado en la fase 3**: es la única parte cuyo costo no puede estimarse sin medir. Señal en el propio repositorio: `config.meta_flac` viene apagado de fábrica mientras `meta_mp3` y `meta_opus` vienen encendidos (`source/config.c`), con interruptor de usuario en ajustes (`source/menus/menu_settings.c:74`) — alguien ya consideró que leer metadatos de FLAC, que es donde vive `FLAC__metadata_get_picture`, era caro.
- **Sin cambios**: `ui/settings` no se toca. La elección de carpeta y el reescaneo son afordancias de la propia pantalla de biblioteca, no una categoría de ajustes; la alternativa se discute en `design.md`.
