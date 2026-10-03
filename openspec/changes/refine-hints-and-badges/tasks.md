## 1. Badges de formato

- [x] 1.1 En `ui_theme.c` / `ui_theme.h`, quitar el parámetro `border` de `UI_DrawBadge`, para que dibuje una sola píldora con `bg` y el texto en `fg`, y agregar `UI_BadgeWidth(ts, label)` (decisión 1). Actualizar las dos llamadas: en `dirbrowse.c`, alineado a la derecha de la fila y con el nombre recortado 16 px antes del badge; en `menu_audioplayer.c`, en su posición actual. Listo cuando `scripts/build.sh` compile con `-Werror` y, en consola, una carpeta con los nueve formatos reconocidos muestre cada nombre legible y sin borde: verde para FLAC y WAV, morado para MP3, OGG y OPUS, ámbar para IT, MOD, S3M y XM, con los badges terminando en la misma columna. También tiene que seguir legible en la fila seleccionada, y en Reproduciendo un FLAC, un MP3 y un XM tienen que mostrar su badge legible.
- [x] 1.2 Agregar a `Menu_DrawLibraryRow` (`menu_library.c`) un parámetro `ext`: `track->ext` en las filas de canción y `NULL` en las de artista y álbum. Dibujar el badge alineado a la derecha y recortar el título y el artista 16 px antes (decisión 2). Listo cuando compile con `-Werror` y, en consola, se cumplan los scenarios de `specs/library/views`: badges en Canciones, en Añadidos recientemente y dentro de un álbum y de un artista; ninguno en las filas de artista ni de álbum; y un título largo recortado antes del badge, con el mini reproductor visible y sin él.

  Resultado de 1.1 y 1.2: verificado en consola con el commit `f94e3f5`. El primer build mostraba el texto descentrado y se corrigió en `36509fa`: vita2d reporta toda línea como `ts` px de alto, así que ahora el texto se centra en la altura de las mayúsculas.

## 2. START apaga la pantalla

- [x] 2.1 Prueba en consola, sin commit: con un build temporal que registre los flancos de `Utils_ReadControls` en `ux0:data/ElevenMPV/`, apagar la pantalla con START en Reproduciendo y encenderla con X, con R y con el botón PS. Anotar acá si el flanco de la pulsación que enciende la pantalla llega a la app. Si no llega, quitar la regla 2 del filtro de `design.md`, el scenario "Encender la pantalla no dispara una acción" de `specs/playback/screen-off` y su caso en 2.2. Listo cuando el resultado esté anotado en esta tarea y los artefactos reflejen ese resultado.

  Resultado (con el build del commit `599513d`, sin build de registro porque lo observado alcanzó para decidir):
  - Con la pantalla apagada por START, la consola solo la vuelve a encender con el botón PS. Ninguna cantidad de pulsaciones de X la encendió.
  - En Reproduciendo, la música se detuvo en esa prueba. No quedó claro si fue por START o por las X.

  Primero se amplió la regla 2: la primera pulsación encendía la pantalla (`scePowerRequestDisplayOn()`) y se descartaba (commit `d81137d`). En consola, la pantalla se encendía al instante, sin que nadie tocara nada, y mostraba la pestaña de desbloqueo. La música siguió sonando en todos los casos. El usuario prefirió el comportamiento anterior, así que la regla 2 se quitó entera: la pantalla queda apagada hasta que se pulsa PS. Según el usuario, las X a ciegas probablemente no llegaban a la app. Qué hacen los botones con la pantalla apagada queda para otro cambio. Se actualizaron la propuesta, el design, el spec (requisito "La pantalla se vuelve a encender con el botón PS") y la prueba en PC.
- [x] 2.2 Crear el módulo puro `source/screen_off.c` / `include/screen_off.h` con `ScreenOff_Filter` (decisión 3), sin vita2d ni SCE. Agregar `tests/test_screen_off.c` a `tests/Makefile`, con estos casos: START apaga y se quita del flanco, L + R + START pasa sin apagar, con `enabled = 0` el flanco pasa intacto, los flancos posteriores al apagado pasan intactos, y un flanco sin START con el filtro desarmado pasa intacto. Listo cuando `make -C tests` pase (con `test_lang`) y el `.vpk` compile con `-Werror` con el módulo agregado a `CMakeLists.txt`.
- [x] 2.3 Aplicar el filtro en `Utils_ReadControls` y llamar ahí a `scePowerRequestDisplayOff()`. Quitar el `break` de START en `menu_displayfiles.c` y en `menu_library.c`, y la llamada de `menu_audioplayer.c`. Deshabilitar el filtro mientras se espera la entrada de texto del filtro. Listo cuando compile con `-Werror` y, en consola, se cumplan los scenarios de "START apaga la pantalla desde cualquier pantalla" y "La pantalla se vuelve a encender con el botón PS" (`specs/playback/screen-off`) y "START ya no cierra la aplicación" (`specs/ui/nav-shell`), en Carpetas, Biblioteca, Reproduciendo, Ajustes y en una pantalla de escaneo, y L + R + START siga cancelando la búsqueda sin apagar la pantalla.
- [x] 2.4 Hacer que `power_tick_thread` (`utils.c`) llame a `sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND)` cuando `Audio_HasTrack() && !Audio_IsPaused()`. Borrar `Utils_LockPower`, `Utils_UnlockPower` y `lock_power` (decisión 4). Listo cuando compile con `-Werror` y, en consola, con el temporizador de ahorro de energía en su valor más corto, se cumpla lo siguiente:
  - Una cola más larga que el temporizador, con la pantalla apagada, suena entera pasando de un track al siguiente.
  - En pausa con la pantalla apagada, la consola se suspende.
  - Con música sonando, el botón PS lleva al menú de la consola y la app se puede cerrar desde la LiveArea.

  Resultado de 2.3 y 2.4: verificado en consola con el commit `f94e3f5`. La pantalla queda apagada hasta que se pulsa PS, y la música sigue sonando en todas las pantallas, Reproduciendo incluida.

## 3. Barra de botones con íconos

- [x] 3.1 Agregar a `ui_theme.h` los tokens `UI_COLOR_BTN_CROSS`, `UI_COLOR_BTN_CIRCLE`, `UI_COLOR_BTN_SQUARE` y `UI_COLOR_BTN_TRIANGLE`, aclarados hasta el contraste de `UI_COLOR_TEXT_SECONDARY` sobre `UI_COLOR_BG_ELEVATED`. Agregar a `nav_rail.c` / `nav_rail.h` el tipo `NavRail_Hint`, el dibujo vectorial de los cuatro símbolos y de los chips neutros, la resolución de confirmar y volver desde `SCE_CTRL_ENTER`, y la regla de no dibujar las entradas que no caben (decisión 5). Todo como una función nueva, junto a la actual. Listo cuando compile con `-Werror` y el contraste de cada token, calculado con la misma fórmula que `UI_ContrastOverBg`, quede anotado junto a su valor en `ui_theme.h`.
- [x] 3.2 Migrar todas las llamadas a la barra (`menu_displayfiles.c`, `menu_library.c`, `menu_audioplayer.c`, `menu_settings.c`, `library.c`) a las entradas de la tabla de la decisión 5, y borrar la función de textos. En `lang_strings.h`:
  - Quitar el nombre del botón de los textos que lo tienen, en los dos idiomas.
  - Borrar `STR_HINT_EXIT`, y también `STR_HINT_MENU` si queda sin uso.
  - Agregar `STR_HINT_SHUFFLE`, `STR_HINT_REPEAT` y `STR_HINT_SCREEN_OFF`.

  Listo cuando compile con `-Werror`, `make -C tests` pase y ninguna línea `X(STR_HINT_...)` de `include/lang_strings.h` nombre un botón (`grep -n "^X(STR_HINT_" include/lang_strings.h | grep "Triangle -\|Square -\|START -\|SELECT -\|L . R -\|L R -\|L + R + START -\|Triángulo -\|Cuadrado -"` no devuelve nada). Los textos del cuerpo de Biblioteca que nombran un botón (`STR_ACTION_*`, `STR_ROOT_MISSING`) no son de la barra y quedan como están.
- [x] 3.3 Verificar en consola los scenarios de "On-screen legend of available physical-button actions" (`specs/ui/nav-shell`), con la consola configurada para confirmar con Cruz y después con Círculo, y en English y en Español:
  - Cada entrada muestra su botón.
  - Confirmar y volver muestran el símbolo que de verdad hace cada cosa.
  - Ninguna entrada ofrece salir.
  - Ninguna entrada se solapa ni se cae, salvo "Apagar pantalla" cuando no cabe.
  - Los cuatro símbolos se distinguen entre sí, y las diagonales del Triángulo y la Cruz se ven sin escalones.

  Listo cuando todos se cumplan en Carpetas, Elegir carpeta, Biblioteca (con contenido y escaneando), Reproduciendo, Ajustes y la entrada de texto del filtro.

  Resultado: verificado en consola con el commit `f94e3f5`, con los colores de los cuatro símbolos aprobados. El texto de la barra y de los chips quedó centrado con los íconos desde `36509fa`.

## 4. Cierre

- [x] 4.1 Agregar a `CHANGELOG.md`, en `[Sin publicar]`, los bullets de las notas de versión de la propuesta, repartidos en `### Agregado`, `### Cambiado` y `### Corregido`, y subir `VITA_VERSION` a `"03.20"` en `CMakeLists.txt`. Listo cuando el `.vpk` compilado tenga `APP_VER 03.20` y la sección muestre todos los bullets de las notas de versión.
- [ ] 4.2 Ejecutar `openspec validate refine-hints-and-badges --strict`. Listo cuando pase y el check `build` del PR de `change/refine-hints-and-badges` hacia `main` esté en verde.
