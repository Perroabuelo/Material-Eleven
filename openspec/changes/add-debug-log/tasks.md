## 1. Lógica pura

- [x] 1.1 Crear `source/log_line.c` e `include/log_line.h`, sin SCE ni vita2d, con el armado de líneas de la Decisión 2: `LogLine_Begin` (tiempo en µs y categoría), `LogLine_Str`, `LogLine_Int` y `LogLine_Buttons` (máscara y tabla de nombres). Incluye el escape de valores y el recorte de UTF-8 con `cut=1` del requirement "Formato de las líneas". Crear `tests/test_log_line.c` con:
  - tiempo en 0, 999 µs, 3 500 000 µs y más de una hora;
  - valores sin comillas, con espacio, con `"`, con `\`, con salto de línea y vacío;
  - recorte de un valor ASCII de 2000 bytes y de otro con caracteres de 2 y 3 bytes cortado en medio de uno: máximo 512 bytes, termina en `cut=1\n`, UTF-8 válido;
  - botones: ninguno, uno y varios, en el orden de la tabla.

  Agregar el test a `BIN` y a las reglas de `tests/Makefile`, y el fuente a `CMakeLists.txt`. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile sin warnings.
- [x] 1.2 Agregar a `log_line.c` el buffer circular de la Decisión 5 (`LogRing_Init` sobre memoria del llamador, `Push` de líneas enteras, `Read` por bloques, totales de aceptadas y descartadas, y la línea `log event=dropped count=N` antes de la siguiente). Agregar también el descarte de estados de la Decisión 6 (`LogState_Changed` por canal y `LogState_Reset`). Ampliar `test_log_line`:
  - llenar, vaciar en parte, dar la vuelta al final del bloque y leer en orden;
  - rechazar y contar una línea que no cabe;
  - la línea `dropped` entra antes que la siguiente con el contador correcto, y si no cabe se sigue contando;
  - igual, distinta y otra vez la primera, como en el escenario "Estado repetido";
  - tras `LogState_Reset`, la siguiente se escribe;
  - camino de error: un buffer de tamaño 0 o menor que una línea rechaza todo sin escribir fuera.

  Listo cuando `make -C tests` pase con sanitizers.

## 2. Módulo de consola

- [ ] 2.1 Crear `source/debug_log.c` e `include/debug_log.h` con `DebugLog_Init` siguiendo los pasos de la Decisión 3 y la API de la Decisión 2 (`DebugLog_On`, `Begin`, `Commit`, `CommitState`, `Screen`, `GetStatus`). Incluye el hilo de la Decisión 4, `DebugLog_Shutdown` y la tabla de botones. Cada paso fallido de `Init` deshace los anteriores, como dice la tabla de Recursos. Pasar `APP_VERSION` desde `VITA_VERSION` con `target_compile_definitions` en `CMakeLists.txt`. Llamar a `DebugLog_Init` en `main.c` después de `sceIoMkdir`, y a `DebugLog_Shutdown` antes de `UI_Debug_Free`. Listo cuando `scripts/build.sh` compile sin warnings y en la consola:
  - sin el marcador no aparecen `debug.log` ni `debug.prev.log` (escenario "Sin marcador no hay log");
  - con el marcador, `debug.log` empieza con la línea `log event=start` con `version` y `time` (escenario "Con marcador el log empieza al arrancar");
  - abrir, cerrar con PS y reabrir deja la sesión anterior en `debug.prev.log` (escenario "Reabrir después de cerrar con PS", por ahora solo con la línea `start`);
  - crear el marcador con la app abierta no crea el log hasta reabrirla.
- [ ] 2.2 Agregar la línea `LOG` a la página de estadísticas del overlay en `ui_gpu.c` (Decisión 9), leyendo `DebugLog_GetStatus`. Listo cuando `scripts/build.sh` compile y en la consola se cumplan los escenarios "Overlay con el log apagado" y "Overlay con el log prendido". Además, una build de prueba no versionada que fuerce la falla del `malloc` en `DebugLog_Init` tiene que mostrar `LOG fallo: sin memoria`, y la app tiene que funcionar normal; esa build no se commitea.

## 3. Puntos de registro

- [ ] 3.1 Registrar los saltos de pantalla con `DebugLog_Screen` en el bucle de `main.c`, con `heap_kb` de `mallinfo`. Registrar los botones en `Utils_ReadControls` y el tacto en `Touch_Update`, según la Decisión 7. Listo cuando `scripts/build.sh` compile y en la consola el log muestre:
  - una línea `screen` por cada salto con el nav rail;
  - `input` con `down=`/`up=` al apretar y soltar, también al apretar dos botones a la vez;
  - el escenario "Toque en la pantalla".
- [ ] 3.2 Agregar las funciones de estado de la Decisión 6 en `menu_displayfiles.c`, `menu_library.c`, `menu_settings.c` y `menu_audioplayer.c`, llamadas una vez por fotograma después del input. Listo cuando `scripts/build.sh` compile y en la consola se cumplan los escenarios "Recorrido corto por la Biblioteca" y "Sin input, sin líneas de estado". Además, en cada pantalla, moverse, entrar en un grupo, abrir el menú de Cuadrado, entrar en modo selección y pausar tienen que producir la línea de estado esperada, y solo una por cambio.
- [ ] 3.3 Registrar `track event=open` en `Audio_Init`, `track event=end` en `Music_Update`, `queue` en `Music_HandleNext`, `playlist` en `PlaylistStore_Save` y `library event=scan` al terminar `Library_RunScan` (Decisión 8). Listo cuando `scripts/build.sh` compile y en la consola se cumplan los escenarios "Pista que no abre", "Fin de pista y avance de la cola" y "Guardar una playlist", y un reescaneo de la biblioteca deje su línea `library event=scan` con `tracks` igual al total que muestra la app.

## 4. Proceso y documentación

- [x] 4.1 Agregar a `rules.design` en `openspec/config.yaml` la regla de la Decisión 10. Listo cuando `openspec validate add-debug-log` pase y `openspec instructions design --change add-debug-log --json` muestre la regla nueva.
- [ ] 4.2 Agregar al `README.md` la sección en inglés de la Decisión 11. Listo cuando la sección explique el marcador, `debug.log`, `debug.prev.log` y el aviso de privacidad.

## 5. Verificación y versión

- [ ] 5.1 Memoria y fluidez en la consola (estrategia de pruebas, pasos 3 y 4). Anotar los resultados en esta tarea. Listo cuando:
  - el escenario "Diez vueltas por las pantallas" no muestre crecimiento del HEAP, comparando las líneas `screen` y el overlay;
  - la diferencia de HEAP entre el log prendido y apagado sea de unos 66 KB;
  - el HEAP sin marcador sea igual al de v4.1.0 (escenario "HEAP sin marcador");
  - mantener abajo 10 s con una pista sonando se vea y suene igual que con el log apagado, anotando si aparece `dropped`;
  - la pila máxima del overlay con el log prendido no se acerque al límite;
  - el escenario "Las líneas llegan a la tarjeta con la app abierta" pase.

  Si algo de esto falla, pausar y revisar con el usuario el riesgo correspondiente de design.md.
- [ ] 5.2 Recorrer en la consola todos los escenarios de `dev/debug-log` que no cubrieron 2.x, 3.x y 5.1, y anotar fecha y resultado en esta tarea. Listo cuando todos pasen, `make -C tests` pase y `scripts/build.sh` compile sin warnings.
- [ ] 5.3 Agregar las notas de versión de `proposal.md` a `[Unreleased]` en `CHANGELOG.md` y llevar `VITA_VERSION` en `CMakeLists.txt` de `"04.10"` a `"04.20"`. Listo cuando `scripts/build.sh` compile y la línea `start` del log diga `version=04.20`.
