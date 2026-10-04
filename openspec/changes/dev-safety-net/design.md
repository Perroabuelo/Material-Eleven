## Context

El porqué está en proposal.md. Acá va el estado que condiciona el cómo.

- **Tests en PC.** `tests/Makefile` compila seis tests con el gcc del host (`-std=gnu11 -O0 -g -Wall -Werror`) y los corre en orden; el primero que falla corta. Los binarios están en `.gitignore`. Cada test enlaza su módulo puro de `source/` directamente.
- **Lo que encuentran los sanitizers hoy.** Compilados fuera del repo con `-fsanitize=address,undefined -fno-sanitize-recover=undefined`, cinco tests pasan y `test_accent` falla: `ACCENT_RGBA8` (`include/accent.h:11`) hace `((a) & 0xFF) << 24` sobre un `int`, y con alfa 128 o más el resultado no cabe. La macro aparece diez veces entre `source/`, `include/` y `tests/`. En la consola, gcc para ARM produce el valor esperado, así que el arreglo no cambia ningún color; lo que cambia es que el código deja de depender de un comportamiento indefinido.
- **CI.** `build.yml` tiene un solo job, dentro del contenedor `vitasdk/vitasdk:2026.08`, que compila el `.vpk`. `release.yml` lo reutiliza con `uses: ./.github/workflows/build.yml`, así que un job nuevo en `build.yml` también corre al publicar una versión.
- **`/scripts/` está en `.gitignore`.** Ahí viven `build.sh` y `vita.ps1`, con rutas de esta máquina. No es lugar para herramientas que se versionan.
- **Volcados de crash.** Un `psp2dmp` es un ELF de 32 bits comprimido con gzip. Sus segmentos `PT_NOTE` traen notas con nombre. Las que sirven, tal como se leyeron el 2026-10-04 en los volcados de la 3.4.2:
  - `APP_INFO`: el Title ID y el nombre de la app (`ELEVENMPV`, o `MPV000001 mpv-vita` en un volcado ajeno que estaba en la misma carpeta).
  - `THREAD_INFO`: `u32` sin uso, `u32` cantidad, y luego una entrada por hilo que empieza con su propio tamaño (`u32`) y su UID, seguidos del nombre. La razón de parada está en el offset `0x74` de la entrada: `0` en los hilos sanos y `0x30004` (data abort) en el que falló.
  - `THREAD_REG_INFO`: misma cabecera; cada entrada es tamaño, UID, `r0` a `r12`, `sp`, `lr`, `pc` y `cpsr`.
  - `MODULE_INFO`: por cada módulo, su nombre y sus segmentos como cuartetos `(atributos, vaddr, tamaño, alineación)`, con atributos `0x5` para código y `0x6` para datos.
- **Resolver direcciones.** El segmento de código del ELF de `~/empv-build/ElevenMPV` se enlaza en `0x81000000`. La consola carga ese segmento donde quiere: el 2026-10-04 lo hizo en `0x81013000` y en `0x8101b000`. La dirección en el ELF es `base_del_ELF + (pc - vaddr_del_segmento_en_el_volcado)`, y `arm-vita-eabi-addr2line -f -i` la traduce a función.

## Goals / Non-Goals

**Goals:**
- Que una fuga, un desborde o un comportamiento indefinido en un módulo puro haga fallar `make -C tests`, en local y en el PR.
- Que el próximo crash en la consola se diagnostique con un solo comando.
- Que los archivos de prueba se regeneren igual en cualquier máquina.

**Non-Goals:**
- Escribir tests nuevos para módulos que no los tienen.
- Que `psp2dmp.py` reconstruya la pila de llamadas. Muestra `pc` y `lr`; desenrollar la pila queda para cuando haga falta.

## Decisions

### 1. Los sanitizers van por defecto en `tests/Makefile`

`CFLAGS` suma `-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined`, y pasa de `-O0` a `-O1` porque es lo que la documentación de ASan recomienda para tener pilas legibles con un costo razonable. Con `-fno-sanitize-recover=undefined`, UBSan corta el test en vez de imprimir un aviso y seguir; sin eso, el test pasaría con el comportamiento indefinido adentro. LeakSanitizer viene con ASan en Linux y revisa las fugas al terminar cada test. Una variable `SANITIZE ?= 1` permite apagarlos con `make -C tests SANITIZE=0`, para cuando haga falta depurar con gdb.

- **Alternativa descartada: un target aparte `make -C tests asan`.** Lo que no corre por defecto, se deja de correr, y es justo lo que le pasó a los tests con el CI.
- **Alternativa descartada: Valgrind.** Encuentra lo mismo que ASan para fugas, pero es mucho más lento, no detecta comportamiento indefinido y habría que instalarlo.

### 2. `ACCENT_RGBA8` desplaza enteros sin signo

Cada componente se convierte a `unsigned int` antes del desplazamiento. El valor resultante es el mismo bit a bit que el que la consola calcula hoy, así que `test_accent` sigue esperando los mismos números.

### 3. Los tests corren en un job propio de `build.yml`

Un job `tests`, en `ubuntu-latest` sin contenedor, con el gcc del runner, que corre `make -C tests`. Va en paralelo al job `build`. Por estar en `build.yml`, corre en cada PR, en cada push a `main` y antes de publicar una versión.

- **Alternativa descartada: correrlos dentro del contenedor de VitaSDK.** No garantiza un gcc del host y ata los tests al mismo ciclo de actualización del toolchain de la consola.

### 4. La regla de memoria va en `rules` de `openspec/config.yaml`

En `rules.design`: si el cambio reserva memoria, abre archivos o crea hilos, el design dice quién libera cada recurso y en qué momento, también cuando algo falla a mitad de camino. En `rules.tasks`: en ese mismo caso, la verificación en consola anota el HEAP del overlay de debug antes y después de repetir la acción N veces, y en los módulos puros, un test de PC cubre el camino de error. En `context`, la línea de pruebas pasa a decir que los tests en PC existen, corren con sanitizers y corren en CI.

- **Alternativa descartada: un documento aparte (CONTRIBUTING).** Las reglas de `config.yaml` son las que el flujo de OpenSpec pone delante al escribir cada artefacto; un documento aparte no lo lee nadie en ese momento.

### 5. Las herramientas van en `tools/` y usan solo la biblioteca estándar

Python 3 sin dependencias, para que corran en WSL, en Windows y en el runner sin instalar nada. Las herramientas externas son opcionales y se buscan en el `PATH` o se pasan por argumento: ffmpeg para los formatos con códec y `arm-vita-eabi-addr2line` para resolver funciones. Si falta alguna, la herramienta hace lo que puede y dice qué se saltó. Ningún archivo de `tools/` tiene rutas de esta máquina.

### 6. `psp2dmp.py` lee el formato tal como se observó

Recibe el volcado y, opcional, el ELF del build. Muestra, en este orden:
1. La app del volcado (`APP_INFO`), para descartar en seguida uno ajeno.
2. Los hilos, con su nombre y su razón de parada, marcando los que no son `0`.
3. Los registros del hilo que falló.
4. Para `pc`, `lr` y cada registro que apunte a un segmento conocido, el módulo, el segmento y el offset.
5. Si recibió el ELF, la función y la línea de `pc` y `lr` con `addr2line`.

Los offsets de la sección Context salen de volcados reales del firmware de esta consola. El script los valida mientras lee (el tamaño de cada entrada, la cantidad declarada contra lo que hay, nombres imprimibles) y, si algo no cuadra, lo dice en vez de mostrar números inventados.

- **Alternativa descartada: vita-parse-core.** Es la herramienta de la comunidad, pero bajarla y ejecutarla fue rechazado por el control de permisos durante `resource-lifecycle-fixes`, y depende de pyelftools. Lo que hace falta acá es poco y cabe en la biblioteca estándar.
- **Los volcados no se versionan.** Traen memoria y datos del sistema de la consola. El script se prueba contra los volcados que están en la tarjeta.

### 7. `gen_testfiles.py` arma una carpeta por propósito

Recibe la carpeta de salida y, opcional, la ruta a ffmpeg. Genera:
- `formatos-mono/` y `formatos-estereo/`: FLAC, MP3, OGG, Opus y WAV con etiquetas; FLAC y MP3 con carátula. El estéreo usa un tono distinto en cada canal, para que se note si se pierde un canal.
- `flac/`: diez FLAC con carátula, para pasar de pista rápido.
- `tracker/`: MOD y XM, como ya hacía.
- `danados/`: una pista buena y archivos de texto con extensión de audio, como ya hacía.
- `nombre/`: el WAV `100% pure %s %d.wav`, como ya hacía.
- `nombres-largos/`: WAV cortos con nombres de 100, 200 y 250 bytes, en ASCII y en UTF-8 con caracteres japoneses, para probar el truncado en pantalla y los límites de ruta (`LIBRARY_PATH_MAX` es 256).
- `letras/`: un WAV con su `.lrc` bien formado, y casos malos (marcas de tiempo inválidas, corchetes sin cerrar, una línea muy larga, un archivo vacío, BOM UTF-8, bytes que no son UTF-8, CRLF).
- `playlists/`: `.m3u` con rutas relativas, absolutas `ux0:`, rutas a archivos que no existen, `#EXTINF`, CRLF, vacía y una con muchas entradas.

Los formatos de `.lrc` y `.m3u` que acepte la app los definirán los cambios de letras y playlists. Estos archivos son la base para probarlos, y cada caso está en una función aparte para que esos cambios los ajusten.

## Risks / Trade-offs

- **[ASan encuentra algo más al sumar un test nuevo]** → Es el objetivo. Lo que encuentre en un módulo existente se corrige en el cambio que lo descubre, o se discute si cambia el alcance.
- **[LeakSanitizer no funciona bajo ptrace ni en algunos contenedores]** → El job corre en el runner sin contenedor, donde funciona. En local funciona en WSL2 (probado el 2026-10-04). Con gdb se usa `SANITIZE=0`.
- **[El formato del volcado cambia con otro firmware]** → El script valida lo que lee y avisa si no cuadra. Los offsets quedan como constantes con nombre al principio del archivo.
- **[Los nombres de 250 bytes no caben en una ruta de Windows]** → Windows limita las rutas a 260 caracteres. El generador avisa si la carpeta de salida es demasiado larga para esos nombres y sugiere una corta, como `C:\pruebas`.
- **[`-O1` cambia lo que ven los tests]** → Ningún test depende del nivel de optimización. Si alguno fallara solo con `-O1`, sería otro comportamiento indefinido que encontrar.

## Migration Plan

No hay datos que migrar. Para volver atrás basta revertir los commits. Las carpetas de prueba que ya están en `ux0:/pruebas-eleven/` siguen sirviendo, y el generador nuevo puede reescribirlas.

## Estrategia de pruebas

- **Compilación:** `scripts/build.sh` sin errores ni avisos con `-Wall -Werror`. Solo cambia `include/accent.h`.
- **Tests en PC:** `make -C tests` pasa con sanitizers. Para comprobar que la red atrapa errores, se agrega a mano y sin commit una fuga (un `malloc` sin `free`) y una escritura fuera de un arreglo en un test, y `make -C tests` tiene que fallar en cada caso.
- **CI:** el PR muestra el job `tests` en verde junto a `build`.
- **Herramientas:**
  - `psp2dmp.py` contra los volcados del 2026-10-04 en `ux0:data/`: en los dos de ElevenMPV tiene que mostrar el data abort del hilo principal y, con el ELF del build de ese día, `_malloc_r` y `_free_r`. En el de mpv-vita tiene que identificar la app ajena.
  - `gen_testfiles.py`: genera todas las carpetas. `ffprobe` confirma los canales de cada formato en las carpetas mono y estéreo.
- **En consola:** reproducir `formatos-estereo/` y `nombres-largos/` sin que la app se caiga. Si `nombres-largos/` descubre un defecto, se anota y se decide si entra en este cambio.

## CI

Un job nuevo, `tests`, en `.github/workflows/build.yml` (decisión 3). El job `build` no cambia.

## Código de terceros

No se incorpora código ni assets de terceros. El formato del volcado se leyó de volcados reales, sin usar código de vita-parse-core.
