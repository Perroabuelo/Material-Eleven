## 1. Tests en PC con sanitizers y en CI

- [x] 1.1 En `include/accent.h`, convertir cada componente de `ACCENT_RGBA8` a `unsigned int` antes del desplazamiento (decisión 2). Listo cuando `scripts/build.sh` compile con `-Wall -Werror`, `make -C tests` pase y `test_accent` compilado a mano con `-fsanitize=address,undefined -fno-sanitize-recover=undefined` ya no informe el desplazamiento de `test_accent.c:75`.
- [x] 1.2 En `tests/Makefile`, compilar por defecto con `-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined` y `-O1`, con `SANITIZE ?= 1` para apagarlos (decisión 1). Listo cuando `make -C tests` pase con los seis tests y, agregando a mano y sin commit primero una fuga y después una escritura fuera de un arreglo en un test, `make -C tests` falle en cada caso; y cuando `make -C tests SANITIZE=0` compile sin sanitizers.
- [x] 1.3 En `.github/workflows/build.yml`, agregar el job `tests` en `ubuntu-latest`, sin contenedor, que corre `make -C tests` (decisión 3). Listo cuando el PR de este cambio muestre el job `tests` en verde junto a `build`.

## 2. Regla de memoria en OpenSpec

- [x] 2.1 En `openspec/config.yaml`, agregar la regla de memoria en `rules.design` y en `rules.tasks`, y actualizar la línea de pruebas del `context` (decisión 4). Listo cuando `openspec instructions design --change dev-safety-net --json` y `openspec instructions tasks --change dev-safety-net --json` muestren las reglas nuevas, y `openspec validate dev-safety-net` pase.

## 3. Herramientas en `tools/`

- [x] 3.1 Crear `tools/crashdump/psp2dmp.py` con la biblioteca estándar de Python (decisión 6). Listo cuando, con los volcados del 2026-10-04 que están en `ux0:data/`, muestre el data abort (`0x30004`) del hilo principal en los dos de ElevenMPV, identifique el de mpv-vita como app ajena y, con un ELF compilado desde el commit de esos volcados (`075741e` o anterior equivalente), resuelva `pc` a `_malloc_r` y `_free_r`.
- [x] 3.2 Crear `tools/testmedia/gen_testfiles.py` a partir del generador de `resource-lifecycle-fixes`, con las carpetas de la decisión 7. Listo cuando genere todas las carpetas en una salida corta (por ejemplo `C:\pruebas`), `ffprobe` informe 1 canal en `formatos-mono/` y 2 en `formatos-estereo/`, avise y salte los formatos con códec si no encuentra ffmpeg, y avise si la carpeta de salida es demasiado larga para `nombres-largos/`.
- [x] 3.3 Escribir `tools/README.md` con qué hace cada herramienta, qué necesita y un ejemplo de uso. Listo cuando cada comando del README, corrido tal cual en WSL, funcione.

## 4. Verificación en consola

- [x] 4.1 Copiar a la Vita las carpetas que genera 3.2 y reproducir `formatos-estereo/` completa y `nombres-largos/` desde Carpetas y desde la Biblioteca. Listo cuando ninguna haga caer la app y el resultado quede anotado en esta tarea. Si `nombres-largos/` descubre un defecto, se anota acá y se decide con el usuario si entra en este cambio.
  - Resultado (2026-10-04, build de `d054600` instalado en la consola; los commits posteriores no tocan código de la app): las carpetas de 3.2 se copiaron a `ux0:/pruebas-eleven/` con robocopy. `formatos-estereo/` completa se reprodujo bien desde Carpetas y desde la Biblioteca, con un tono distinto en cada canal y carátula en FLAC y MP3. `nombres-largos/` (100, 200 y 250 bytes, ASCII y japonés) se reprodujo bien desde Carpetas y desde la Biblioteca. El color de acento se ve igual que antes. La app no se cayó en ningún caso y `nombres-largos/` no descubrió defectos.
