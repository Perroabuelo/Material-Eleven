## 1. Lógica pura y tests en PC

- [x] 1.1 Agregar `TrackAdvance_PrevRestarts` y `TRACK_ADVANCE_RESTART_MS` a `include/track_advance.h` y `source/track_advance.c` (D1), con sus casos en `tests/test_track_advance.c` (0, 3000, 3001 y `UINT32_MAX` ms). Listo cuando `make -C tests` pase con sanitizers.
- [x] 1.2 Crear `source/fade.c` e `include/fade.h` con la rampa de ganancia y la interpolación del master (D2), y `tests/test_fade.c` con los casos de la estrategia de pruebas (44,1 y 48 kHz, mono y estéreo, invertir a medias, saturación, atajo con ganancia 1, master continuo entre buffers). Agregarlo a `tests/Makefile`. Listo cuando `make -C tests` pase con sanitizers.
- [x] 1.3 Crear `source/sleep_timer.c` e `include/sleep_timer.h` (D4), y `tests/test_sleep_timer.c` con reloj falso: vencimiento de cada opción una sola vez, ganancia de los últimos 10 s, minutos redondeados hacia arriba, reemplazar y cancelar durante el fundido, `SleepTimer_TrackEnded`, salto de reloj y nombres del log. Agregarlo a `tests/Makefile`. Listo cuando `make -C tests` pase con sanitizers.

## 2. Audio

- [ ] 2.1 Generar con ffmpeg en Windows los archivos de prueba de design.md en `ux0:/pruebas-eleven/niceties/` (`01`, `02` y `03 tono 8s.flac`, `04 tono 60s.flac`). Listo cuando los cuatro suenen en la consola con la versión actual y `debug.log` tenga para cada uno una línea `track` `event=open` `result=ok`.
- [ ] 2.2 En `source/audio/audio.c` e `include/audio/audio.h`: `pause_request`, `Audio_TogglePause`, `Audio_IsPaused` como intención, `Audio_IsSilent`, la rampa de 100 ms en `Audio_Decode` (D2), `Audio_Hold` (D3), `Audio_SetMasterGain` con interpolación por buffer (D4) y `Audio_SetStartPaused` consumida en `Audio_Init` antes del callback (D4). Borrar `Audio_Pause`. Agregar `fade.c` y `sleep_timer.c` a `CMakeLists.txt`. En `menu_audioplayer.c`, `Music_SeekTo` con `Audio_Hold`, usado por `Menu_HandleSeekTouch`, y los tres llamadores de la pausa del usuario pasan a `Audio_TogglePause`. Listo cuando `scripts/build.sh` compile y, en la consola, con canciones de `ux0:/Mis cosas/` y auriculares, pausar y reanudar suene sin clic, pausar y reanudar enseguida no corte el sonido, buscar con la barra sonando y en pausa no cambie el estado de pausa, y cada archivo de `ux0:/pruebas-eleven/formatos-mono/` y `formatos-estereo/` suene completo sin cortes.

## 3. Retroceder

- [ ] 3.1 En `source/menus/menu_audioplayer.c`, hacer de `Music_Previous` el único camino de retroceder (D1): L en Reproduciendo y el botón de anterior en pantalla pasan a llamarla. Reiniciar con `Music_SeekTo(0.0)`, llevar la letra sincronizada al principio y registrar `track` `event=restart`. Listo cuando `scripts/build.sh` compile y, en la consola, con `04 tono 60s.flac`, L después de 5 s reinicie la pista con una línea `event=restart` y sin línea `queue`, L antes de 1 s pase a la anterior, y lo mismo desde el botón en pantalla y el del mini reproductor.

## 4. Temporizador

- [ ] 4.1 En `menu_audioplayer.c`, el estado del temporizador y `Music_SleepTick` llamado desde `Utils_ReadControls` junto a `Music_Update`, con el fundido final, la pausa al vencer, el master que vuelve a 1 cuando la pausa se cumplió, y el modo de fin de pista en `Music_Update` con `Audio_SetStartPaused`, borrando la marca al volver de `TrackAdvance_Run` (D4). Registrar `sleep` `event=set` y `event=expire`. Por ahora, elegir la opción solo desde un valor fijo de prueba. Listo cuando `scripts/build.sh` compile y, en la consola, el modo de fin de pista con `01 tono 8s.flac` deje `02 tono 8s.flac` en pausa en 0:00 sin que suene, con las líneas del escenario "Vencer al terminar la pista" de `specs/dev/debug-log`. Quitar el valor fijo de prueba al terminar.
- [ ] 4.2 El menú del temporizador con SELECT en las tres vistas (D5): dibujo, botones, toques, bloqueo del resto del input mientras está abierto, cierre al salir de Reproduciendo, la leyenda con la entrada de SELECT y la leyenda del menú abierto (D7), y los campos `menu` y `sleep` al final de `nowplaying` (D8). Textos nuevos en `include/lang_strings.h`. Listo cuando `make -C tests` pase (test_lang), `scripts/build.sh` compile y, en la consola, el escenario "Elegir y cancelar el temporizador" de `specs/dev/debug-log` produzca sus líneas en orden.
- [ ] 4.3 El indicador en `source/status_bar.c` (D6), con la luna dibujada como geometría y los minutos o "Fin de pista". Listo cuando `scripts/build.sh` compile y, en la consola, el indicador muestre "15 min" al elegir 15, "Fin de pista" y "End of track" en el modo de fin de pista según el idioma, en Reproduciendo, Carpetas y Biblioteca, sin solaparse con la hora ni con la batería, y desaparezca al cancelar.

## 5. Suite de consola

- [ ] 5.1 Escribir `tests/console/playback-niceties.txt` (formato de `dev/console-scripts`) con los escenarios de la tabla de la estrategia de pruebas de design.md, incluido el de 15 minutos con `timeout=960s`, y sumarla a la tabla de `tests/console/README.md`. Listo cuando la suite corra en la consola y `debug.log` termine con `script` `event=end` sin escenarios fallados, y el resultado quede anotado aquí.

## 6. Documentación y versión

- [ ] 6.1 En `README.md`, documentar en inglés la regla de retroceder, el temporizador (SELECT en Now Playing, las opciones, el fundido y el modo de fin de pista, el indicador) y el fundido de pausa. Listo cuando el README lo explique.
- [ ] 6.2 Subir la versión a 4.5.0 (`VITA_VERSION "04.50"` en `CMakeLists.txt`) y agregar la entrada de v4.5.0 a `CHANGELOG.md` con las notas de versión de proposal.md. Listo cuando `scripts/build.sh` genere el `.vpk` y su `param.sfo` diga `04.50`.

## 7. Verificación en consola

- [ ] 7.1 Recorrer en la consola, paso a paso, todos los escenarios de `specs/playback/queue`, `specs/playback/sleep-timer`, `specs/playback/pause-fade`, `specs/ui/now-playing` y `specs/dev/debug-log` de este cambio, en español y en inglés, incluidos los manuales de la estrategia de pruebas (fundidos de oído, indicador, leyenda, menú sobre letras y visualizador, fin de pista con la pantalla apagada, reabrir desde la LiveArea, reiniciar un archivo de cada formato de `formatos/`). Listo cuando todos se cumplan y el resultado quede anotado aquí.
- [ ] 7.2 Borrar de la tarjeta `ux0:/pruebas-eleven/niceties/` y todo lo generado para estas pruebas, conservando el resto del material de prueba anterior. Listo cuando la carpeta ya no exista en la consola.
