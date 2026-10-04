Rama: `change/flat-screen-dispatch`, creada desde `main`. Su PR apunta a `main`.

## Why

Cada cambio de pantalla (nav rail, SELECT, volver, reproducir una canción) se hace llamando a la función de la pantalla nueva desde dentro del lazo de la anterior, y esa llamada nunca vuelve. Por eso la pila del hilo principal crece con cada salto y no baja nunca. En una sesión larga, con mucha navegación, termina desbordándose: la app se cae o corrompe memoria vecina sin un patrón claro. `fix-back-to-origin` dejó anotado este riesgo para un cambio aparte, y la revisión de memoria del 2026-10-03 lo confirmó como el único recurso que crece sin límite en uso normal.

## What Changes

- **Las pantallas dejan de llamarse entre sí.** Cada pantalla corre su lazo hasta que el usuario elige otra y entonces *devuelve* cuál es la siguiente. Un despachador central abre esa pantalla. La profundidad de la pila queda fija, cambie el usuario de pantalla las veces que cambie.
- **Reproducir ya no abre Reproduciendo desde adentro de la pantalla de origen.** Carpetas y Biblioteca cargan la pista y piden ir a Reproduciendo. La ida y la vuelta se ven igual que hoy, incluido volver a la pantalla donde se eligió la canción.
- **El overlay de debug muestra la pila usada** por el hilo principal, para poder comprobar en la consola que no crece.
- **El listado de Carpetas se libera sin recursión.** Hoy se libera con una llamada por archivo, hasta 1024 marcos encima de la pila.
- No cambia nada visible: las pantallas, los botones, el nav rail y la leyenda funcionan igual.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `ui/nav-shell`: se agrega el requisito de que cambiar de pantalla no acumule memoria, por muchas veces que se haga.

## Impact

- **Código C**:
  - `source/main.c`: el despachador reemplaza la llamada única a `Menu_DisplayFiles()`.
  - `source/menus/menu_displayfiles.c`, `menu_library.c`, `menu_settings.c` y `menu_audioplayer.c`: cada lazo devuelve la pantalla siguiente en vez de llamarla.
  - `source/dirbrowse.c`: `Dirbrowse_OpenFile` pide ir a Reproduciendo en vez de entrar; la liberación del listado pasa a ser iterativa.
  - `source/ui_gpu.c`: línea nueva en el overlay de debug.
  - Módulo puro nuevo para el pedido de cambio de pantalla, con su prueba en PC.
- **APIs**: `Menu_DisplayFiles`, `Menu_DisplayLibrary`, `Menu_DisplaySettings` y `Menu_ShowNowPlaying` pasan a devolver `UI_Screen`. `Menu_PlayAudio` y `Menu_PlayQueued` dejan de bloquear hasta salir de Reproduciendo.
- **Textos, assets, dependencias y build**: sin cambios, salvo agregar la prueba nueva a `tests/Makefile`.
- **CI**: sin cambios.

## Fuera de alcance

- Las fugas de memoria del ciclo de vida del audio y de los decoders. Van en `resource-lifecycle-fixes`.
- Que la cola avance sola a la pista siguiente cuando el usuario está en otra pantalla. Hoy solo avanza con Reproduciendo abierto, y eso se mantiene.
- Una pila de navegación general (volver desde Ajustes a la pantalla anterior, etc.).
- La carátula a resolución completa de la pista en curso y el llenado de los atlas de fuentes TTF.

## Notas de version

Versión objetivo: **v3.4.1** (patch): corrige un crash que aparecía tras cambiar de pantalla muchas veces.

- Fixed a crash that could happen after switching between screens many times in a long session.
