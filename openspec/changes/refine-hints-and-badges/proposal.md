Rama: `change/refine-hints-and-badges`, creada desde `main`. Su PR apunta a `main`.

## Why

Hay tres cosas de la interfaz que no cumplen lo que prometen:

- **Los badges de formato no se leen.** En Carpetas y en Reproduciendo se ven como una píldora de color sólido, sin la extensión adentro. La paleta ya es la de `pic0.png`, pero el texto es invisible.
- **START cierra la app sin aviso.** En Carpetas y en Biblioteca, START sale de la app, aunque haya música sonando. En Reproduciendo, en cambio, apaga la pantalla.
- **La barra de abajo no dice qué botón hace cada cosa.** Dice "Abrir / Reproducir" o "Atrás" sin indicar el botón, y otras veces lo nombra con palabras ("Triángulo - Reescanear").

Además, revisando START apareció un defecto escondido. Desde `6070b10` la app ya no impide que la consola se suspenda sola mientras reproduce. Ese commit quitó el bloqueo del botón PS, y con él se perdió sin querer la señal que mantenía despierta a la consola. Con la pantalla apagada, lo esperable es que la música se corte cuando venza el temporizador de ahorro de energía del sistema.

## What Changes

- **Badge legible, como en `pic0.png`**: fondo tenue del color del tipo de formato, sin borde, con la extensión en el color sólido (verde para FLAC y WAV, morado para MP3, OGG y OPUS, ámbar para IT, MOD, S3M y XM). Se arregla en Carpetas y en Reproduciendo.
- **Badges en las filas de canciones de Biblioteca**: en la vista Canciones, en Añadidos recientemente y dentro de un artista o un álbum. Las filas que representan un artista o un álbum no llevan badge.
- **START apaga la pantalla en todas las pantallas** y la reproducción sigue. **BREAKING** para quien usaba START para salir: la app se cierra como cualquier app de la consola, desde el botón PS y la LiveArea. L + R + START sigue cancelando la búsqueda.
- **Encender la pantalla no dispara una acción**: la pulsación que vuelve a encender la pantalla no llega a la app como una orden, por ejemplo, no abre la fila seleccionada.
- **La consola no se suspende sola mientras suena un track**, sin volver a bloquear el botón PS. En pausa o sin nada cargado, la consola vuelve a suspenderse con normalidad.
- **Barra de botones con íconos**: cada acción muestra el ícono de su botón. Cruz, Círculo, Cuadrado y Triángulo se dibujan como geometría, con los colores de PlayStation (azul, rojo, rosa y verde) ajustados para leerse sobre el fondo oscuro. L, R, SELECT y START van en chips neutros con su nombre. Los íconos de confirmar y de volver siguen la asignación de la consola: si la consola confirma con Círculo, la barra muestra Círculo.
- **Los textos de la barra dejan de nombrar el botón**: "Triángulo - Reescanear" pasa a "Reescanear" junto al ícono, en inglés y en español. Desaparece "START - Salir".

## Capabilities

### New Capabilities
- `playback/screen-off`: reproducir con la pantalla apagada. Cubre qué apaga la pantalla, que la reproducción siga y la consola no se suspenda mientras suena un track, que el botón PS siga funcionando, y que encender la pantalla no dispare ninguna acción.

### Modified Capabilities
- `ui/nav-shell`: START deja de salir de la app, y la leyenda de botones pasa a mostrar el ícono del botón de cada acción, siguiendo la asignación de confirmar y volver de la consola.
- `ui/folder-browser`: el badge de formato tiene que mostrar la extensión de forma legible, con el color de su familia de formatos.
- `ui/now-playing`: el mismo requisito de legibilidad para el badge de formato de Reproduciendo.
- `library/views`: las filas de canciones de la biblioteca muestran el badge de formato.

## Impact

- **Código C**:
  - `source/ui_theme.c`: `UI_DrawBadge` y sus llamadas en `source/dirbrowse.c` y `source/menus/menu_audioplayer.c`.
  - `source/menus/menu_library.c`: el badge en `Menu_DrawLibraryRow`.
  - `source/nav_rail.c` / `include/nav_rail.h`: la barra recibe pares de botón y texto, y dibuja los íconos.
  - Todas las llamadas a la barra: `menu_displayfiles.c`, `menu_library.c`, `menu_audioplayer.c`, `menu_settings.c` y `library.c`.
  - `source/utils.c`: START, la pulsación que enciende la pantalla y la señal de mantenerse despierto.
  - Se quitan los `break` de START en `menu_displayfiles.c` y `menu_library.c`.
- **Textos**: `include/lang_strings.h`, en los dos idiomas. Se quita `STR_HINT_EXIT`, y los textos que nombran el botón pierden ese prefijo.
- **Assets, dependencias y build**: ninguno nuevo. Los íconos son geometría con las primitivas que ya existen.
- **CI**: sin cambios.

## Fuera de alcance

- Una opción en Ajustes para salir de la app, o una confirmación al salir.
- Cambiar qué hace START mientras está abierta la entrada de texto de la búsqueda.
- Badges en las filas de artista o de álbum, o en el mini reproductor.
- Cambiar la paleta de familias de formatos o sumar formatos nuevos.
- Íconos para el pad direccional o los sticks en la barra.
- Apagar la pantalla desde un control táctil.

## Notas de version

Versión objetivo: **v3.2.0** (minor): agrega los íconos de botones en la barra, los badges en Biblioteca y la reproducción con la pantalla apagada desde cualquier pantalla.

- La barra de abajo muestra el ícono de cada botón, con sus colores, y respeta si tu consola confirma con Círculo o con Cruz.
- Los badges de formato ahora muestran la extensión (FLAC, MP3, OGG...), y también aparecen en las canciones de la Biblioteca.
- START apaga la pantalla desde cualquier pantalla y la música sigue sonando. Para cerrar la app, usa el botón PS.
- La consola ya no se suspende sola mientras suena música con la pantalla apagada.
- Al encender la pantalla de nuevo, ese botón no abre ni cambia nada por accidente.
