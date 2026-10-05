## 1. Etapa 1: instrumentaci贸n (una sola compilaci贸n para el A/B)

- [x] 1.1 Crear el m贸dulo puro `source/capture_bmp.c` / `include/capture_bmp.h`, que arma la cabecera BMP de 32 bits y convierte una fila ABGR a BGRA (decisi贸n 4), y su test `tests/test_capture_bmp.c` agregado a `tests/Makefile`. El test comprueba la cabecera (tama帽o de archivo, offset de p铆xeles, 32 bpp, alto y ancho), la inversi贸n de filas y la conversi贸n de canales sobre una imagen de 3x2 conocida. Listo cuando `make -C tests` pase con ASan y UBSan, y `scripts/build.sh` compile con `-Wall -Werror`.
- [ ] 1.2 Agregar `UI_GpuBeginFrame()` a `ui_gpu.c`/`ui_gpu.h`, con `frame_sync` apagado por defecto, y reemplazar las 11 llamadas a `vita2d_start_drawing()` de `menu_audioplayer.c`, `menu_displayfiles.c`, `menu_library.c`, `menu_settings.c` y `library.c` (decisi贸n 1). Listo cuando `grep -rn vita2d_start_drawing source` muestre una sola llamada (dentro de `ui_gpu.c`), compile con `-Werror`, y en consola las cuatro pantallas y el escaneo de biblioteca se dibujen igual que antes.
  - C骴igo en `b370e96`, compila con `-Werror`. Las llamadas resultaron ser 10, no 11: la und閏ima coincidencia del conteo era un comentario en `menu_audioplayer.c`. Falta la verificaci髇 en consola.
- [ ] 1.3 Agregar el modo captura al overlay (`UI_DEBUG_CAPTURE` en el ciclo de L + R + SELECT): reservar el buffer al entrar y liberarlo al salir y en `UI_Debug_Free`, `UI_Debug_ArmCapture()` llamada al final de `Music_HandleNext`, registrar el framebuffer y `pool_used` en `UI_Debug_Draw` y copiar con un fotograma de retraso en `UI_Debug_Update` (decisiones 2 y 5). El panel muestra el estado de la captura y los `pool_used` de N+1 y N+2. Listo cuando compile con `-Werror` y en consola, tras un cambio de pista con el modo abierto, el panel pase a `lista` y muestre dos valores de pool.
  - C骴igo en `f67a69e`, compila con `-Werror`. Falta la verificaci髇 en consola.
- [ ] 1.4 Agregar los controles del modo captura: Arriba alterna `frame_sync` (el panel muestra `SYNC encendido/apagado`) y SELECT, con flanco de subida y sin L ni R, guarda `glitch_NNN.bmp` y `glitch_NNN.txt` en `ux0:data/ElevenMPV/glitch/` usando `capture_bmp`, con `sceIoClose` en todos los caminos (decisiones 3, 4 y 5). Listo cuando en consola SELECT produzca un `.bmp` que se abre en el PC y muestra Now Playing correctamente, el `.txt` traiga los valores del panel, y un segundo SELECT cree `glitch_001` sin pisar `glitch_000`.
  - C骴igo en `ed8edce`, compila con `-Werror`. Falta la verificaci髇 en consola.
- [ ] 1.5 Verificaci贸n de memoria del modo captura. Anotar la l铆nea HEAP del overlay en estad铆sticas, entrar y salir del modo captura 10 veces guardando una captura en cada vuelta, y volver a anotar HEAP. Listo cuando el HEAP final coincida con el inicial (con una tolerancia de pocos KB) y el resultado quede anotado en esta tarea.

## 2. Experimento en consola

- [ ] 2.1 Con MSAA 4x (comprobar en el panel de estad铆sticas), modo captura abierto y `SYNC apagado`: 70 cambios con R en Now Playing desde Biblioteca. Contar los glitches y guardar con SELECT al menos dos capturas de cambios con glitch. Listo cuando el conteo, los nombres de las capturas y sus `pool_used` queden anotados en esta tarea.
- [ ] 2.2 Mismo protocolo con `SYNC encendido`. Listo cuando el conteo quede anotado en esta tarea.
- [ ] 2.3 Analizar en el PC las capturas de 2.1: anotar si lo roto empieza despu茅s del primer elemento que cambia de estructura (la barra de progreso u otro), y si los `pool_used` de N+1 y N+2 difieren. Aplicar el criterio de decisi贸n de `design.md` (0/70 confirma; 1 o 2 indica otra causa adicional; unos 6 descarta). Listo cuando la conclusi贸n quede anotada aqu铆. **Si la hip贸tesis no se confirma, el change se detiene ac谩**: no se hace la etapa 2 y se revisa `design.md` con el usuario.

## 3. Etapa 2: el arreglo (solo si 2.3 confirma)

- [ ] 3.1 Volver incondicional la espera en `UI_GpuBeginFrame()`: quitar `frame_sync` y el control de Arriba, y dejar en el panel de captura solo la captura y los valores de pool. Actualizar el comentario de cabecera de `ui_gpu.h` para registrar esta tercera aparici贸n de la misma clase de fallo. Listo cuando compile con `-Werror`, `make -C tests` pase y el panel de captura ya no ofrezca el interruptor.
- [ ] 3.2 Verificaci贸n en consola del escenario "Cambio de track sin artefactos visuales": 70 cambios con R, con MSAA 4x, sin el modo captura abierto. Listo cuando el resultado sea 0/70 y quede anotado.
- [ ] 3.3 Verificaci贸n en consola de rendimiento y regresi贸n: Carpetas, Biblioteca, Now Playing y Ajustes siguen a 60 fps (mismo m茅todo que `close-ui-contract` 2.2), el escaneo de biblioteca se dibuja bien, y el escenario "Cambio repetido de track", alternando pistas con y sin car谩tula, no tira la app. Listo cuando los tres resultados queden anotados.

## 4. Cierre

- [ ] 4.1 Agregar el bullet de `proposal.md` (Notas de version) a `CHANGELOG.md` bajo `[Unreleased]` y subir `VITA_VERSION` a `03.43` en `CMakeLists.txt`. Listo cuando CI del PR est茅 en verde. Solo si se hizo la etapa 3; si el change se detuvo en 2.3, no se toca ninguno de los dos.
