## 1. Lógica pura de repetición

- [ ] 1.1 Crear `include/key_repeat.h` y `source/key_repeat.c` con `KeyRepeat_State`, `KeyRepeat_Step`, `KeyRepeat_Update` y `KeyRepeat_Move`, y las constantes de retardo (400 ms), intervalo (66 ms) y paso a páginas (2000 ms) (Decisiones 2 y 3), sin vita2d ni SCE. Crear `tests/test_key_repeat.c` con todos los casos de "Estrategia de pruebas" de design.md y agregarlo a `BIN` y a las reglas de `tests/Makefile`. Agregar `source/key_repeat.c` a `CMakeLists.txt`. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile sin warnings.

## 2. Lectura del pad

- [ ] 2.1 En `source/utils.c`, alimentar un `KeyRepeat_State` estático en `Utils_ReadControls` con los mantenidos (sin START), el flanco ya filtrado y `sceKernelGetProcessTimeWide()`, y exponer `Utils_ListStep(int *sel, int count, int page)` en `include/utils.h` (Decisiones 1, 3 y 4). `pressed` no cambia. Listo cuando `scripts/build.sh` compile sin warnings y la app se comporte igual que antes en la consola (todavía ninguna lista usa el helper).

## 3. Listas

- [ ] 3.1 Biblioteca: en `Menu_HandleLibraryControls` (`source/menus/menu_library.c`), cambiar ↑/↓ con `Utils_SetMax`/`Utils_SetMin` por `Utils_ListStep(&selection, count, ROWS_PER_PAGE)`; ←/→ siguen igual. Listo cuando `scripts/build.sh` compile y en la consola, con una biblioteca de al menos 500 canciones, se cumplan en la vista Canciones los escenarios "Una pulsación corta mueve una sola fila", "Cruzar una lista larga", "Soltar devuelve el paso de una fila" y "El salto de página no se pasa del borde" de `ui/list-navigation`, y "Mantener R en la Biblioteca" y "Mantener confirmar sobre una canción".
- [ ] 3.2 Carpetas: en `source/menus/menu_displayfiles.c`, usar `Utils_ListStep(&position, visible_count, FILES_PER_PAGE)` en `Menu_HandleControls` y en el bucle de elegir carpeta de la biblioteca. Listo cuando `scripts/build.sh` compile y en la consola se cumplan "Mantener ↓ recorre la lista sola", "Mantener ↓ se detiene en la última fila" y "Una pulsación nueva en el borde da la vuelta" en Carpetas, y mantener ↓ también recorra la lista al elegir la carpeta de la biblioteca.
- [ ] 3.3 Ajustes: en `source/menus/menu_settings.c`, usar `Utils_ListStep(&item_index, cat->item_count, cat->item_count)` para los ítems; L/R siguen una vez por pulsación. Listo cuando `scripts/build.sh` compile y en la consola se cumpla "Mantener ↑ en Ajustes", y mantener L o R cambie de categoría una sola vez.
- [ ] 3.4 Verificación cruzada en la consola: escenario "Mantener ↑ en Reproduciendo" de `ui/list-navigation`, y que el filtro de Carpetas (IME) y START para apagar la pantalla funcionen igual que antes mientras se usa la repetición. Ajustar las constantes de 1.1 si el ritmo no se siente bien, sin tocar los tests. Listo cuando todos los escenarios de `ui/list-navigation` se hayan comprobado en la consola y el resultado quede anotado en esta tarea.

## 4. Versión y cierre

- [ ] 4.1 Agregar los bullets de Notas de versión de `proposal.md` a `CHANGELOG.md` bajo `[Unreleased]` (en `### Added`) y subir `VITA_VERSION` a `03.70` en `CMakeLists.txt`. Listo cuando `scripts/build.sh --clean` compile sin warnings y el `.vpk` instalado encima de v3.6.0 muestre `03.70` y conserve la configuración y el índice.
- [ ] 4.2 Push de `change/key-repeat` y PR a `main`. Listo cuando el CI del PR esté en verde (build del `.vpk` y tests en PC, incluido `test_key_repeat`).
