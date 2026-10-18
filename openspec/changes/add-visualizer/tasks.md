## 1. Comprobar lo incierto antes de construir

- [ ] 1.1 Generar con ffmpeg en Windows los archivos de prueba de la estrategia de pruebas de design.md (`tono-100hz.flac`, `tono-8khz.flac`, `tono-440hz.flac`, `solo-izquierdo.wav`, `pulsos.wav`) y copiarlos a `ux0:/pruebas-eleven/visualizador/`. Listo cuando los cinco suenen en la consola con la versión actual y `debug.log` tenga para cada uno una línea `track` `event=open` `result=ok` con el `channels` esperado.
- [ ] 1.2 Prueba provisional del disco (D4): en la vista normal, dibujar la carátula como abanico texturizado de 96 segmentos con `vita2d_draw_array_textured` y UV giradas, con el anillo de suavizado encima. Listo cuando en la consola se vea un círculo girando con la carátula de un FLAC de `ux0:/pruebas-eleven/flac/`, con el borde liso, y el overlay muestre el uso del pool sin agotamientos. Anotar el resultado aquí y quitar el código provisional.
- [ ] 1.3 Prueba provisional de pantalla apagada (D8): registrar un callback de energía y anotar en `debug.log` qué eventos llegan al apagar la pantalla con START y encenderla con PS. Listo cuando quede anotado aquí si el callback informa el encendido o si hace falta el respaldo del salto de tiempo entre fotogramas, y se quite el registro provisional.

## 2. Lógica pura y tests en PC

- [x] 2.1 Crear `source/viz_tap.c` e `include/viz_tap.h` con el buffer circular de un escritor y un lector, la conversión de mono a estéreo, la lectura con retraso, el descarte si el escritor dio la vuelta y el reinicio (D1, D2), y `tests/test_viz_tap.c`. Agregarlo a `tests/Makefile`. Listo cuando `make -C tests` pase con sanitizers.
- [x] 2.2 Crear `source/viz_fft.c` e `include/viz_fft.h` (FFT radix-2 de 1024 puntos con ventana de Hann, D3) y `tests/test_viz_fft.c`, que compara con una DFT directa y comprueba los bins de 100 Hz, 1 kHz y 8 kHz a 44,1 y 48 kHz. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.3 Crear `source/viz_bands.c` e `include/viz_bands.h` (32 bandas logarítmicas de 40 Hz a 16 kHz en dB, y energía de graves con ganancia automática, D3) y `tests/test_viz_bands.c`. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.4 Crear `source/viz_meter.c` e `include/viz_meter.h` (ataque y caída que dependen de `dt`, pico con retención, RMS por canal en dB, D3) y `tests/test_viz_meter.c`, con el caso de 60 frente a 30 fps. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.5 Crear `source/viz_scope.c` e `include/viz_scope.h` (disparo en un cruce ascendente, D3) y `tests/test_viz_scope.c`. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.6 Crear `source/viz_spin.c` e `include/viz_spin.h` (giro con inercia, D4), `source/viz_disc.c` e `include/viz_disc.h` (posiciones y UV del abanico, D4) y sus tests `tests/test_viz_spin.c` y `tests/test_viz_disc.c`. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.7 Crear `source/viz_nav.c` e `include/viz_nav.h` con las transiciones de vista y de estilo (D6), y `tests/test_viz_nav.c` con todas las transiciones de los escenarios de `specs/ui/visualizer` y `specs/ui/now-playing`. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.8 Agregar a `source/gesture.c` el deslizamiento horizontal `GESTURE_SWIPE_X` (D7), con sus casos en `tests/test_gesture.c`, incluido que un arrastre vertical no se dispare como deslizamiento. Listo cuando `make -C tests` pase con sanitizers y los casos anteriores de `test_gesture` sigan pasando.

## 3. Toma de audio

- [ ] 3.1 En `source/audio/audio.c`, escribir cada buffer de `Audio_Decode` (también el silencio) en `viz_tap`, con el ancho de frame de `vitaAudioGetChannelCount()`, y reiniciar el buffer circular en `Audio_Init` antes de `vitaAudioInit` (D1). Agregar los fuentes nuevos de `source/` a `CMakeLists.txt`. Listo cuando `scripts/build.sh` compile y, en la consola, cada archivo de `ux0:/pruebas-eleven/formatos-mono/` y `formatos-estereo/` suene completo de principio a fin, sin cortes y sin que la app se cierre.

## 4. Vista y estilos

- [ ] 4.1 En `include/nav_rail.h` y `source/nav_rail.c`, agregar `HINT_BTN_DOWN` y `HINT_BTN_LEFT_RIGHT` dibujados como chips con flechas (D9). En `include/lang_strings.h`, agregar `STR_HINT_VISUALIZER` y `STR_HINT_STYLE` en inglés y español. Listo cuando `make -C tests` pase (test_lang) y `scripts/build.sh` compile.
- [ ] 4.2 En `source/menus/menu_audioplayer.c`, reemplazar `lyrics_view` por `np_view` y `viz_style` usando `viz_nav` (D6). Agregar la vista de visualizador con la cabecera (sin la carátula en pequeño en los estilos de vinilo), el indicador de puntos, su barra de progreso, ← y →, el deslizamiento en la zona, y la leyenda de las tres vistas (D9). Agregar `view` y `style` al final de la línea `nowplaying`. Listo cuando `scripts/build.sh` compile y, en la consola, ↓, ↑, ← y → produzcan las líneas `nowplaying` del escenario nuevo de `specs/dev/debug-log`, con la vista de letras funcionando igual que antes.
- [ ] 4.3 Crear `source/viz_draw.c` e `include/viz_draw.h` con el análisis por fotograma (ventana desde `viz_tap` con el retraso de D2) y los estilos Espectro, Osciloscopio y VU, con el color de acento. Listo cuando `scripts/build.sh` compile y, en la consola, `tono-100hz.flac` encienda la izquierda del Espectro, `tono-8khz.flac` la mitad derecha, `tono-440hz.flac` dibuje una onda quieta en el Osciloscopio, y `solo-izquierdo.wav` mueva solo el medidor L.
- [ ] 4.4 Agregar `UI_DrawRingN` a `source/ui_theme.c` sin cambiar `UI_DrawRing`. En `viz_draw.c`, agregar el vinilo (etiqueta texturizada, surcos, agujero, anillos de suavizado y etiqueta lisa sin carátula) y los estilos Disco + espectro radial y Disco que respira (D4, D5). Listo cuando `scripts/build.sh` compile y, en la consola, el vinilo gire con la carátula de un FLAC de `ux0:/pruebas-eleven/flac/`, frene al pausar y acelere al reanudar, un archivo de `tracker/` muestre la etiqueta lisa, y el overlay no registre agotamientos del pool con el estilo radial.
- [ ] 4.5 Pantalla apagada: no analizar mientras la app tenga la pantalla apagada y retomar al encenderla, con la vía elegida en 1.3, liberando el callback al cerrar (D8, "Recursos y su liberación"). Listo cuando `scripts/build.sh` compile y, en la consola, apagar la pantalla un minuto con el visualizador abierto y encenderla con PS muestre lo que suena en ese momento, con la música sin cortes.
- [ ] 4.6 Ajustar `VIZ_HW_LATENCY_FRAMES` con `pulsos.wav` en el estilo Espectro, con el altavoz y con auriculares de cable. Listo cuando las barras suban a la vez que se oye cada pitido, y el valor elegido quede anotado aquí y en design.md (Open Questions).

## 5. Suite de consola

- [ ] 5.1 Escribir `tests/console/ui-visualizer.txt` (formato de `dev/console-scripts`) con los escenarios de navegación de la estrategia de pruebas de design.md: abrir y cerrar, salto entre letras y visualizador, los cinco estilos con la vuelta, flechas en la vista normal, memoria al ir a Ajustes, estilo tras R y memoria (`heap_kb`). Listo cuando la suite corra en la consola y `debug.log` termine con `script` `event=end` sin escenarios fallados.

## 6. Documentación y versión

- [ ] 6.1 En `README.md`, documentar el visualizador: cómo abrirlo, cómo cambiar de estilo, los cinco estilos y las dos limitaciones (no refleja el EQ ni el ALC, y con auriculares Bluetooth puede verse adelantado). Listo cuando el README lo explique en inglés.
- [ ] 6.2 Subir la versión a 4.4.0 (`VITA_VERSION "04.40"` en `CMakeLists.txt`) y agregar la entrada de v4.4.0 a `CHANGELOG.md` con las notas de versión de proposal.md. Listo cuando `scripts/build.sh` genere el `.vpk` y su `param.sfo` diga `04.40`.

## 7. Verificación en consola

- [ ] 7.1 Recorrer en la consola, paso a paso, todos los escenarios de `specs/ui/visualizer`, `specs/ui/now-playing`, `specs/ui/nav-shell` y el escenario nuevo de `specs/dev/debug-log`, en español y en inglés, con `ux0:/pruebas-eleven/visualizador/`, `formatos/`, `formatos-mono/`, `flac/` y `tracker/` y con canciones de `ux0:/Mis cosas/`. Listo cuando todos se cumplan y el resultado, con las cifras de heap y el uso del pool del estilo radial, quede anotado aquí.
- [ ] 7.2 Borrar de la tarjeta `ux0:/pruebas-eleven/visualizador/` y todo lo generado para estas pruebas, conservando el resto del material de prueba anterior. Listo cuando la carpeta ya no exista en la consola.
