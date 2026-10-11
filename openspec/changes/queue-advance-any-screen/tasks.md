## 1. Decisión pura del avance

- [x] 1.1 Crear `include/track_advance.h` y `source/track_advance.c` con `TrackAdvance_Ops` y `TrackAdvance_Run` (Decisión 2), sin vita2d ni SCE. Crear `tests/test_track_advance.c` con los casos de la estrategia de pruebas, con dobles que abren o fallan según una tabla y cuentan los intentos, y agregarlo a `BIN` y a las reglas de `tests/Makefile`. Listo cuando `make -C tests` pase con sanitizers, incluido el caso en que ninguna pista abre (devuelve 0 después de exactamente `count` intentos).

## 2. Un solo punto para el fin de pista

- [x] 2.1 En `source/menus/menu_audioplayer.c`, hacer que `Music_HandleNext` use `TrackAdvance_Run` con adaptadores sobre `Menu_InitMusic`, `Queue_Advance` y `Queue_PeekAhead`, conservando el desmontaje, `UI_Debug_ArmCapture` y `track_failed`. Agregar `source/track_advance.c` a `CMakeLists.txt`. Listo cuando `scripts/build.sh` compile sin warnings, `make -C tests` siga en verde y en la consola L, R, el transporte táctil y el mini reproductor cambien de pista como antes.
- [x] 2.2 Agregar `Music_Update()` (Decisión 1, con la condición `Audio_HasTrack()`) y declararla en `include/menus/menu_audioplayer.h`. Quitar la comprobación de `!playing` de `Menu_RunNowPlayingLoop` y llamar `Music_Update()` al principio de `Utils_ReadControls()` (`source/utils.c`). Listo cuando `scripts/build.sh` compile y en la consola se cumplan, con las pistas cortas de `ux0:/pruebas-eleven/formatos/`, estos escenarios de `playback/queue`:
  - "La cola sigue en Carpetas";
  - "La cola sigue en la Biblioteca y en Ajustes";
  - "La repetición se respeta fuera de Reproduciendo";
  - "La cola sigue con la pantalla apagada fuera de Reproduciendo";
  - "Reproduciendo sigue avanzando como antes".

## 3. Regresión en consola

- [x] 3.1 Con el overlay de debug abierto y en Carpetas (no en Reproduciendo), anotar el HEAP, dejar terminar solas diez pistas cortas de `ux0:/pruebas-eleven/formatos/` y volver a la primera. Comprobar también:
  - el escenario "Escuchar un álbum entero con la pantalla apagada" de `playback/screen-off`, empezado desde Carpetas;
  - que una pista que termina durante un escaneo de la biblioteca da paso a la siguiente sin cortar el escaneo;
  - que una pista que termina con el teclado de búsqueda abierto da paso a la siguiente sin cerrar el teclado.

  Listo cuando el HEAP final sea el anotado (con no más de unos pocos KB de diferencia), las tres comprobaciones se cumplan y los valores queden anotados aquí.

  Resultado (2026-10-11, consola, v03.51):
  - HEAP: 1560 al empezar y 1560 después de dos vueltas a la cola (12 cambios automáticos en Carpetas). Sin diferencia.
  - Álbum de `ux0:/pruebas-eleven/flac/` con la pantalla apagada desde Carpetas: dio la vuelta completa sin cortes.
  - Escaneo de la biblioteca: pasaron varias pistas mientras escaneaba, sin cortar el escaneo.
  - Teclado de búsqueda abierto: pasaron varias pistas sin cerrar el teclado, y al cerrarlo el mini reproductor mostraba el título nuevo.

## 4. Versión y cierre

- [x] 4.1 Agregar el bullet de Notas de versión de `proposal.md` a `CHANGELOG.md` bajo `[Unreleased]` (sección `### Fixed`) y subir `VITA_VERSION` a `03.51` en `CMakeLists.txt`. Listo cuando `scripts/build.sh --clean` compile sin warnings y el `.vpk` instalado encima de v3.5.0 muestre `03.51` y conserve la configuración y el índice.
- [x] 4.2 Push de `change/queue-advance-any-screen` y PR a `main`. Listo cuando el CI del PR esté en verde (build del `.vpk` y tests en PC, incluido `test_track_advance`).
