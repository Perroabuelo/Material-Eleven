## Why

La cola solo avanza sola en Reproduciendo. Si el usuario vuelve a Carpetas, a la Biblioteca o a Ajustes mientras suena una pista, al terminar esa pista no empieza la siguiente y la app queda en silencio. Se confirmó en la consola el 2026-10-09. El paso a la siguiente pista solo existe dentro del bucle de dibujo de Reproduciendo (`menu_audioplayer.c`, después de `vita2d_swap_buffers`), y ninguna otra pantalla lo hace.

Además, como la pista sigue cargada y sin pausa, el power tick sigue impidiendo la suspensión automática: la consola se queda encendida y callada hasta que el usuario toca algo.

El defecto apareció en el spike de `background-playback`. Ese cambio se cerró sin integrarse (ver su design.md, "Resultado del spike"), y esta corrección pasa a un cambio propio.

## What Changes

- **La cola avanza desde cualquier pantalla.** Cuando una pista termina sola, se repite si la repetición está encendida y si no se pasa a la siguiente del orden vigente. Eso ocurre en Reproduciendo, Carpetas, Biblioteca y Ajustes, también con la pantalla apagada con START. Ya no depende de que Reproduciendo esté abierta.
- **El fin de pista se atiende en un solo lugar,** por el que pasan todas las pantallas una vez por fotograma. Reproduciendo deja de tener su propia comprobación.
- **La decisión de qué hacer al terminar una pista** (repetir, avanzar, saltar pistas que no abren, probar como mucho una vuelta, descargar si nada abre) pasa a un módulo de lógica pura con tests en PC. Así se puede probar el caso "ninguna pista abre", que en la consola no se puede provocar.
- **Si nada abre fuera de Reproduciendo,** la reproducción se detiene y la pista se descarga sin cambiar de pantalla: el mini reproductor desaparece. En Reproduciendo el comportamiento no cambia: sale de la pantalla como hasta ahora.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `playback/queue`: el fin de una pista avanza la cola (o la repite) desde cualquier pantalla, no solo desde Reproduciendo. Se define también qué pasa fuera de Reproduciendo cuando ninguna pista de la cola se puede abrir.

## Impact

- **`source/menus/menu_audioplayer.c`**: `Music_HandleNext` usa el módulo nuevo. Se agrega `Music_Update()`, que reemplaza la comprobación de `!playing` de `Menu_RunNowPlayingLoop`. Se declara en `include/menus/menu_audioplayer.h`.
- **`source/utils.c`**: `Utils_ReadControls()` llama `Music_Update()`. Por ahí pasan Carpetas, Biblioteca, Ajustes, Reproduciendo, el bucle del diálogo de búsqueda y los bucles de escaneo de `library.c`.
- **Archivos nuevos**: `source/track_advance.c`, `include/track_advance.h` y `tests/test_track_advance.c`. Se agregan a `CMakeLists.txt` y a `tests/Makefile`.
- **Versión y documentación**: `VITA_VERSION` a `03.51` y CHANGELOG.
- Sin dependencias, licencias ni assets nuevos. El CI solo suma el test nuevo.

## Rama

`change/queue-advance-any-screen`

## Fuera de alcance

- **Reproducir con la app en segundo plano o con un juego abierto.** Es lo que buscaba `background-playback`, cerrado sin integrarse. Con categoría `gd` la consola congela el proceso en segundo plano, y eso no cambia aquí.
- **Cambiar la categoría, el presupuesto de memoria o el manejo del puerto BGM.**
- **Mostrar un mensaje cuando ninguna pista abre.** Se mantiene el silencio que ya define `playback/queue`.
- **Cambiar el orden de la cola, el barajado o la repetición.** Solo cambia desde dónde se atiende el fin de pista.

## Notas de version

Versión objetivo: **v3.5.1** (patch: corrección).

- Fixed: when a track ended while you were on Folders, Library or Settings, the next one did not start. The queue now moves on, or repeats the track, from any screen, including with the screen turned off.
