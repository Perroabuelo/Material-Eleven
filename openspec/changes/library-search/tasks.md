## 1. Diálogo de búsqueda compartido

- [ ] 1.1 Crear `source/search_dialog.c` e `include/search_dialog.h` con `SearchDialog_Prompt` (Decisión 1). Mover sin cambios de comportamiento, desde `menu_displayfiles.c`:
  - el lazo del IME;
  - las constantes `FILTER_*` y sus comentarios;
  - `Menu_OpenFilterDialog` y el color de fondo estático;
  - el abandono con drenaje;
  - el dibujado de fondo, por callback, con la leyenda de cancelar.

  Todavía con la conversión de texto actual. Carpetas pasa a llamar al módulo y aplica el filtro solo con `CONFIRMED`. Agregar el fuente a `CMakeLists.txt`. Listo cuando:
  - `scripts/build.sh` compile sin warnings;
  - en la consola se cumplan los escenarios de "El usuario nunca queda atrapado en el filtro" de `ui/folder-browser`: descartar, filtro sin coincidencias en la raíz y volver por debajo de la raíz;
  - y mantener L+R+START cierre el diálogo con la reproducción sonando.

  Anotar el HEAP del overlay de debug antes y después de abrir y cerrar el diálogo 10 veces.

## 2. Texto: conversión y comparación

- [x] 2.1 Mover `Menu_Utf8ToUtf16` a `text_encoding.c` como función pública sobre `uint16_t` y agregar su inversa `Utf16ToUtf8`, con pares sustitutos, sustituto suelto como U+FFFD y sin cortar secuencias (Decisión 3). Ampliar `tests/test_text_encoding.c` con: ida y vuelta de "acción", pares sustitutos, sustituto suelto, buffer justo y buffer que cortaría una secuencia. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.2 Usar las dos conversiones en `search_dialog.c` para el título, el término inicial y el resultado, y quitar la copia byte a byte y el `(char)input[j]`. Listo cuando:
  - `scripts/build.sh` compile;
  - y en la consola se cumpla el escenario "Un término con tildes no se desfigura" de `ui/folder-browser`: escribir "acción", confirmar, ver "acción" en la caja y volver a abrir el diálogo con "acción" cargado.
- [x] 2.3 Crear `source/text_fold.c` e `include/text_fold.h` con `TextFold_Fold` y `TextFold_Contains` (Decisión 2), sin SCE ni vita2d. Crear `tests/test_text_fold.c` con los casos de "Estrategia de pruebas" de design.md y agregarlo a `BIN` y a las reglas de `tests/Makefile`. Agregar el fuente a `CMakeLists.txt`. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile.
- [ ] 2.4 `Dirbrowse_NameContains` pasa a usar `TextFold_Contains`. El término se pliega una vez en `Dirbrowse_SetFilter`, guardando el original para mostrarlo. Listo cuando `scripts/build.sh` compile y en la consola, con `ux0:/pruebas-eleven/` y un archivo "Canción.flac", se cumplan:
  - "Filtrar sin tildes encuentra nombres con tildes";
  - y "Filtering narrows the current listing", sin regresiones.

## 3. Triángulo en Carpetas

- [ ] 3.1 △ abre el filtro en la pantalla principal de Carpetas, y la leyenda suma △ junto a "Buscar" con el nuevo `STR_HINT_SEARCH` en `lang_strings.h`. Listo cuando:
  - `make -C tests` pase (`test_lang`);
  - `scripts/build.sh` compile;
  - y en la consola se cumplan "Triángulo abre el filtro" de `ui/folder-browser` y "Buscar aparece en Carpetas" de `ui/nav-shell`, con la leyenda sin solaparse en English y en Español.

## 4. Lista filtrada en la Biblioteca

- [x] 4.1 Crear `source/list_filter.c` e `include/list_filter.h` con `ListFilter_Build`, `ListFilter_Count`, `ListFilter_ToFull` y `ListFilter_ToVisible`, sobre un arreglo estático de `LIBRARY_MAX_TRACKS` (Decisión 4). Crear `tests/test_list_filter.c` con los casos de design.md y agregarlo a `tests/Makefile`. Agregar el fuente a `CMakeLists.txt`. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile.
- [ ] 4.2 En `menu_library.c`, hacer que toda la pantalla trabaje en índices visibles a través de dos `ListFilter` (exterior y de grupo), todavía sin término, como identidad: dibujado, contador, ↑↓←→, entrar y reproducir. Reproducir traduce con `ListFilter_ToFull` (Decisión 6). Listo cuando `scripts/build.sh` compile y en la consola la Biblioteca se comporte exactamente como antes en las cuatro vistas:
  - entrar y salir de un artista y de un álbum;
  - reproducir;
  - volver desde Reproduciendo a la misma fila.

## 5. Búsqueda en la Biblioteca

- [ ] 5.1 Agregar los términos (original y plegado) y las funciones de coincidencia por lista (Decisión 5), con el ciclo de vida de la Decisión 7. △ abre `SearchDialog_Prompt` solo en `LIBRARY_STATE_READY`, y el botón de volver deshace lo más reciente primero. Agregar `STR_SEARCH_LIBRARY` y `STR_NO_RESULTS` a `lang_strings.h` y dibujar el placeholder sin resultados. Listo cuando `make -C tests` pase (`test_lang`), `scripts/build.sh` compile, y en la consola se cumplan los escenarios de `library/views`:
  - de "La biblioteca permite buscar en la lista que se ve", todos menos el de la caja tocable;
  - y todos los de "El usuario nunca queda atrapado en la búsqueda de la biblioteca".

  Anotar el HEAP antes y después de buscar y quitar el término 10 veces.
- [ ] 5.2 Dibujar la caja de búsqueda en la barra superior de la Biblioteca, que muestra el término o "Buscar en la biblioteca", y abrir el diálogo al tocarla (Decisión 8). Listo cuando `scripts/build.sh` compile y en la consola se cumplan:
  - el escenario "Abrir la búsqueda tocando la caja";
  - y que, en English y en Español, la caja, el título, las cuatro pestañas y la ruta de la carpeta se vean sin solaparse, también dentro de un álbum con nombre largo.
- [ ] 5.3 Comprobar la cola y el regreso con un término aplicado. Listo cuando en la consola se cumplan:
  - "Reproducir un resultado de búsqueda" y "Reproducir un resultado dentro de un álbum" de `library/views`, este último también con "Seguir con el siguiente";
  - y "Volver a la Biblioteca con una búsqueda aplicada" de `ui/nav-shell`.

## 6. Menú de Cuadrado

- [ ] 6.1 Implementar el menú superpuesto de □ con "Reescanear" y "Elegir carpeta" (Decisión 8): ↑↓, confirmar, volver, toque sobre una opción y toque fuera. Reescanear y elegir carpeta borran los términos y salen del grupo. △ deja de reescanear. La leyenda de Biblioteca queda en △ "Buscar", □ "Opciones" (`STR_HINT_OPTIONS` reemplaza a `STR_HINT_FOLDER`) y volver como "Quitar filtro" o "Volver" según el caso. Agregar `STR_MENU_RESCAN` y `STR_MENU_CHOOSE_FOLDER` a `lang_strings.h`. Listo cuando `make -C tests` pase, `scripts/build.sh` compile y en la consola se cumplan:
  - todos los escenarios de "Reescanear y cambiar de carpeta se piden desde la biblioteca" de `library/views`;
  - y "Cada acción muestra su botón" de `ui/nav-shell`, en los dos idiomas.

## 7. Versión y cierre

- [ ] 7.1 Agregar los bullets de Notas de versión de `proposal.md` a `CHANGELOG.md` bajo `[Unreleased]` (en `### Added`, `### Changed` y `### Fixed`) y subir `VITA_VERSION` a `03.80` en `CMakeLists.txt`, o a la que corresponda si `key-repeat` no salió antes. Listo cuando `scripts/build.sh --clean` compile sin warnings y el `.vpk` instalado encima de la versión anterior muestre la nueva y conserve la configuración y el índice.
- [ ] 7.2 Push de `change/library-search` y PR a `main`. Listo cuando el CI del PR esté en verde: build del `.vpk` y tests en PC, incluidos `test_text_fold`, `test_list_filter` y `test_text_encoding`.
