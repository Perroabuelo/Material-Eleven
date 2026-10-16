## 1. Lógica pura

- [x] 1.1 Crear `source/selection.c` e `include/selection.h`, sin SCE ni vita2d, con el bitmap estático de la Decisión 1: `Selection_Clear`, `Toggle`, `Set`, `Has`, `Count`, `AllState`, `ToggleAll` y `Next`. Crear `tests/test_selection.c` con los casos de la estrategia de pruebas de design.md:
  - posiciones 0, 3999 y fuera de rango;
  - `AllState` con filtro vacío, ninguna, algunas, todas y con marcas ocultas;
  - la secuencia de "queen" y "abba" con `ToggleAll`, que no toca lo oculto;
  - `Next` en orden.

  Agregar el test a `BIN` y a las reglas de `tests/Makefile`, y el fuente a `CMakeLists.txt`. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile sin warnings.
- [ ] 1.2 Agregar `Selection_Step` (Decisión 4): recorrido sobre la casilla y las filas visibles, con vuelta por la casilla en pulsación nueva y detención en `1` y `count` en pasos repetidos. Ampliar `test_selection` con los dos bordes en pulsación nueva y repetida, la vuelta completa y `count == 0`. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 1.3 Toque mantenido en `gesture.c` (Decisión 2): `hold_ms` en `Gesture_Input`, `held_ms` en el estado, `GESTURE_HOLD` único que marca `caught`, y `GESTURE_HOLD_MS` = 500. Ampliar `tests/test_gesture.c`:
  - mantener por encima del umbral da un `HOLD` y luego `RELEASE`;
  - levantar antes da `TAP`;
  - arrastrar antes da `DRAG` sin `HOLD`;
  - `hold_ms = 0` nunca da `HOLD`;
  - un toque `caught` no da `HOLD`;
  - arrastrar después del `HOLD` da `DRAG`.

  Los casos que ya existían deben seguir pasando sin cambios. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile.
- [ ] 1.4 Agregar `Playlist_AppendFrom` (Decisión 6) en `playlist.c`: reserva única, `PathIndex` sobre las entradas previas, fuente por callback y liberación del índice en todos los caminos. Agregar `path_index.c` al test de playlist en `tests/Makefile`. Ampliar `test_playlist`:
  - orden, omitidas y conteos;
  - 3998 + 5 llena;
  - fuente que corta antes de `n`;
  - fallo inyectado en la reserva de entradas y en la del índice, que dejan la playlist igual y sin fugas.

  Listo cuando `make -C tests` pase con sanitizers.
- [ ] 1.5 Agregar `Playlist_RemoveIf` (Decisión 7). Ampliar `test_playlist` con ninguna, la primera, alternadas, la última y todas, comprobando el orden que queda, el conteo y `*first`. Listo cuando `make -C tests` pase con sanitizers.

## 2. Táctil

- [ ] 2.1 En `touch_list.c`: el campo `hold_ms` (0 por defecto, que `TouchList_Reset` conserva) pasado a `Gesture_Input`, y `TouchList_HeldRow`. Listo cuando:
  - `scripts/build.sh` compile sin warnings;
  - y en la consola Carpetas, elegir carpeta y Ajustes se comporten igual que antes: mantener un segundo sobre una carpeta y levantar entra en ella, y arrastrar y deslizar siguen iguales.

## 3. Modo selección en la biblioteca

- [ ] 3.1 Estado del modo (Decisión 3): `selecting`, `marks`, `header_focus` y `Menu_LibraryEndSelect`, llamada desde volver sin término, `Menu_LibraryExit`, `Menu_LibraryFreeOpenPlaylist`, `SetView`, `Enter`, `Leave` y `Rescan`. Además:
  - `OPTION_SELECT` en el menú de Cuadrado según el delta de `library/views` (no en la pestaña Playlists), con `OPTIONS_MAX` = 5;
  - el toque mantenido de la biblioteca (`hold_ms = GESTURE_HOLD_MS`), que entra al modo con la fila marcada, o no hace nada en la pestaña Playlists;
  - textos nuevos en `lang_strings.h`.

  Listo cuando `scripts/build.sh` compile, `test_lang` pase y en la consola:
  - "Seleccionar" aparezca en el orden que piden los escenarios del delta de `library/views`;
  - mantener sobre una canción entre al modo sin reproducir;
  - mantener sobre una playlist de la pestaña Playlists no haga nada.
- [ ] 3.2 Botones y dibujo del modo (Decisiones 5 y 10): `Menu_HandleLibrarySelect` con `Selection_Step`, `Toggle` y `ToggleAll`, volver en dos tiempos, Triángulo, y L, R y SELECT sin efecto; casilla en cada fila; encabezado con salir, contador, caja de búsqueda y casilla Todo de tres estados, todo tocable; leyenda del modo. Listo cuando `scripts/build.sh` compile y en la consola se cumplan los escenarios de `library/multi-select`:
  - marcar con botones, con toque y manteniendo;
  - la casilla Todo llena, parcial y deshabilitada sin resultados;
  - el recorrido por la casilla, y mantener arriba sin entrar en ella;
  - "queen" y "abba" con el contador correcto;
  - volver en dos tiempos;
  - L, R, SELECT y el nav rail;
  - la leyenda del modo, en los dos idiomas.

## 4. Acciones sobre lo marcado

- [ ] 4.1 Menú del modo y añadir (Decisiones 6 y 9): menú de una sola acción, deshabilitada sin marcas, y la fuente sobre las marcas para canciones y grupos, en dos pasadas para los grupos. La conectan `AddToExisting` y `AddToNew`, que salen del modo solo si se escribió. Listo cuando `scripts/build.sh` compile y en la consola se cumplan los escenarios de añadir del delta de `library/playlists` y de `library/multi-select`:
  - tres canciones marcadas en otro orden;
  - dos álbumes;
  - "queen" y "abba" intercaladas;
  - lo oculto que también se añade;
  - cerrar el selector y descartar el nombre, que conservan las marcas;
  - el aviso único.
- [ ] 4.2 Quitar varias (Decisiones 7 y 8): diálogo con tipo `CONFIRM_REMOVE_MARKED` que abre en cancelar, `Playlist_RemoveIf` con un solo `Save`, salida del modo y fila actual según la Decisión 7. Listo cuando `scripts/build.sh` compile y en la consola se cumplan los escenarios del delta de `library/playlists`:
  - quitar 3 de 10, con la tercera seleccionada en el segundo puesto;
  - el diálogo que abre en cancelar;
  - quitar canciones atenuadas;
  - quitar todas;
  - reiniciar la aplicación y ver los cambios guardados.

  Borrar una playlist debe seguir funcionando igual.

## 5. Verificación de conjunto y cierre

- [ ] 5.1 Memoria y tiempo en la consola (estrategia de pruebas, pasos 6 y 7). Anotar el HEAP del overlay de debug, repetir 10 veces marcar todo en Canciones, añadirlo a una playlist nueva y borrarla, y anotar el HEAP de nuevo; debe quedar igual. Medir lo que tarda añadir todo a una playlist que ya tiene la mitad. Anotar los resultados en esta tarea. Listo cuando el HEAP no crezca y añadir no deje la pantalla congelada más de un segundo; si la deja, pausar y revisar con el usuario el riesgo de design.md.
- [ ] 5.2 Recorrer en la consola todos los escenarios de consola de las specs del cambio que no cubrieron 3.x y 4.x, y anotar fecha y resultado en esta tarea. Listo cuando todos pasen, `make -C tests` pase y `scripts/build.sh` compile sin warnings.
- [ ] 5.3 Agregar las notas de versión de `proposal.md` a `[Unreleased]` en `CHANGELOG.md` y llevar `VITA_VERSION` en `CMakeLists.txt` de `"04.00"` a `"04.10"`. Listo cuando `scripts/build.sh` compile y el `.vpk` muestre `04.10`.
- [ ] 5.4 Abrir el PR de `change/multi-select` contra `main`. Listo cuando el CI esté en verde.
