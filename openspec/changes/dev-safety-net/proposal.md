Rama: `change/dev-safety-net`, creada desde `main`. Su PR apunta a `main`.

## Why

En `resource-lifecycle-fixes` (v3.4.2) todos los defectos tenían el mismo origen: código que toma o escribe memoria sin que nada lo compruebe. Las fugas no se veían en el overlay, y los desbordes, como el del MP3 mono, solo aparecían con un tipo de archivo concreto y se diagnosticaron leyendo a mano los volcados de crash de la consola. Lo que viene (playlists y letras) es justamente código que parsea archivos y reserva memoria por pista. Hoy no hay ninguna red que atrape esos errores antes de la consola:

- Los tests en PC de `tests/` se corren solo a mano. El workflow de CI compila el `.vpk` pero no los ejecuta, aunque `add-ci-build` dejó anotado que se sumarían.
- Los tests no usan sanitizers. Corridos con AddressSanitizer y UBSan ya fallan: `ACCENT_RGBA8` (`include/accent.h`) desplaza un `int` 24 bits, que es comportamiento indefinido con alfa 128 o más, y se usa también en la app.
- Ninguna regla de OpenSpec pide decir quién libera lo que un cambio reserva, ni medir el heap en la consola.
- Las herramientas que sirvieron para el diagnóstico (lector de volcados `psp2dmp` y generador de archivos de prueba) quedaron fuera del repo y se pierden.

## What Changes

- **Tests en PC con sanitizers.** `make -C tests` compila cada test con `-fsanitize=address,undefined`, de modo que una fuga, una escritura fuera de buffer o un comportamiento indefinido en un módulo puro hace fallar el test. Se corrige `ACCENT_RGBA8`, que es lo primero que esto encuentra.
- **Los tests corren en CI.** Un job nuevo en `.github/workflows/build.yml` ejecuta `make -C tests` en cada PR y en cada push a `main`. Un PR con un test fallido queda en rojo.
- **Regla de memoria en `openspec/config.yaml`.** Si un cambio reserva memoria, abre archivos o crea hilos, su design dice quién libera cada cosa y cuándo, y su estrategia de pruebas mide el HEAP del overlay de debug antes y después de repetir la acción N veces. También se actualiza la línea del contexto que dice que los tests en PC "se agregan cuando existan".
- **Herramientas versionadas en `tools/`.** `/scripts/` está en `.gitignore` a propósito (scripts locales con rutas fijas), así que las herramientas van en una carpeta nueva:
  - `tools/crashdump/psp2dmp.py`: lee un volcado `psp2dmp` y muestra la app, los hilos con su razón de parada, los registros del hilo que falló y el módulo y offset de cada dirección. Si recibe el ELF del build, resuelve las direcciones con `addr2line`.
  - `tools/testmedia/gen_testfiles.py`: genera las carpetas de prueba para la consola. Conserva lo que ya hacía (MOD, XM, WAV, archivos dañados y un nombre con `%`) y suma variantes estéreo además de mono, archivos `.lrc` bien y mal formados, playlists `.m3u` con rutas inexistentes, y nombres de archivo muy largos. FLAC, MP3, OGG y Opus se generan con ffmpeg cuando está disponible.
  - Un `tools/README.md` explica cómo usar cada una.

## Capabilities

### New Capabilities

Ninguna. El cambio es de herramientas y proceso: no altera el comportamiento del reproductor, así que se declara `skip_specs: true`. La corrección de `ACCENT_RGBA8` no cambia ningún color en la consola.

### Modified Capabilities

Ninguna.

## Fuera de alcance

- Tests en PC nuevos para módulos que todavía no los tienen. Cada cambio futuro (playlists, letras) agrega los suyos y ya corren con sanitizers.
- Sanitizers o detección de fugas en la consola. El overlay con la línea HEAP sigue siendo la herramienta allí.
- Análisis estático o lint de C.
- Revisar el código de vita2d o de las librerías de `libs/`. Su macro `RGBA8` tiene el mismo desplazamiento, pero no se compila en los tests de PC.
- Leer volcados de otras apps. La herramienta muestra la app del volcado para descartarlos, y nada más.

## Notas de version

- Versión objetivo: **la próxima versión**, sin salto propio. El cambio no tiene efecto visible dentro de la app, así que no agrega bullets a `CHANGELOG.md` ni sube `VITA_VERSION` al archivarse.

## Impact

- **Código C:** `include/accent.h`, solo la macro `ACCENT_RGBA8`.
- **Tests:** `tests/Makefile` (flags de sanitizer).
- **CI:** `.github/workflows/build.yml` (job nuevo de tests).
- **OpenSpec:** `openspec/config.yaml` (regla de memoria y contexto actualizado).
- **Archivos nuevos:** `tools/README.md`, `tools/crashdump/psp2dmp.py` y `tools/testmedia/gen_testfiles.py`.
- **APIs, textos, assets y dependencias de la app:** sin cambios. Las herramientas usan solo la biblioteca estándar de Python 3, más ffmpeg y `arm-vita-eabi-addr2line` como opcionales.
