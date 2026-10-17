## 1. Spike

- [x] 1.1 Comprobar `sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL)` en la consola con una build de prueba no versionada: con el marcador del log, un combo temporal relanza la app. Probarlo en Carpetas sin pista, y en la Biblioteca con una pista sonando y el overlay abierto. Anotar en esta tarea el código de retorno, si el proceso nuevo arranca en Carpetas sin música, si el `debug.log` del proceso anterior tiene todas sus líneas (con un `DebugLog_Sync` provisional) y el `heap_kb` de la primera línea `screen` de cada proceso. Listo cuando los tres casos relancen limpio. Si alguno falla, pausar y revisar con el usuario el plan B de design.md (Decisión 6) antes de seguir. La build de prueba no se commitea.

  Resultados (2026-10-17), con una build que no rotaba `debug.log` para que los cuatro procesos quedaran en un solo archivo:
  - Código de retorno: `sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL)` no volvió en ninguno de los tres casos. La línea `spike event=returned` que seguía a la llamada nunca se escribió, y la ruta alternativa `app0:/eboot.bin` no hizo falta.
  - Los tres casos relanzaron limpio: Carpetas sin pista (`track=0`), con una pista sonando (`track=1`) y con una pista sonando y el overlay abierto (`track=1`). En los dos últimos, la app estaba en Reproduciendo después de abrir la pista desde la Biblioteca. Cada proceso nuevo empezó con `log event=start` y `screen to=folders`, sin música y sin líneas `track` hasta que se abrió una pista.
  - Log del proceso anterior: la última línea de cada uno es la `spike event=relaunch`, escrita con el `DebugLog_Sync` provisional justo antes de la llamada. No falta ninguna.
  - `heap_kb` de la primera línea `screen` de cada uno de los cuatro procesos: 1362.

## 2. Lógica pura

- [x] 2.1 Agregar `LogLine_Parse` a `log_line.c` (Decisión 1): separa tiempo, categoría y campos, y deshace comillas y escapes. Ampliar `tests/test_log_line.c` con ida y vuelta contra `LogLine_Str` para valores con espacio, `"`, `\` y salto de línea, valor vacío, una línea con `cut=1` y líneas mal formadas (sin categoría, comillas sin cerrar), que se rechazan sin leer fuera del buffer. Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.2 Crear `source/script.c` e `include/script.h` con `Script_Parse` y los límites de la Decisión 1 y de Recursos (64 KB, 1024 pasos, un solo bloque). Crear `tests/test_script.c` con los casos de parseo y de error de la estrategia de pruebas, incluidos los escenarios "Guion válido" y "Botón desconocido", y el camino de error con 64 KB de basura y con más de 1024 pasos. Agregarlo a `BIN` y a las reglas de `tests/Makefile`, y el fuente a `CMakeLists.txt`. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile sin warnings.
- [ ] 2.3 Agregar a `script.c` el `ScriptRun` con los tiempos de la Decisión 3 (`press`, `hold`, `tap`, `swipe`, `sleep`, `seed`) y el `wait` de la Decisión 5 (lookahead, estado actual, timeout con `line` y `last`). Ampliar `test_script` con reloj falso: los casos del runner y del `wait` de la estrategia de pruebas, y los escenarios "Campos de más en la línea" y "Valor distinto". Listo cuando `make -C tests` pase con sanitizers.
- [ ] 2.4 Agregar `SuiteState_Format` y `SuiteState_Parse` (Decisión 4) con su test: ida y vuelta, y un archivo truncado o con basura que se trata como ausente. Listo cuando `make -C tests` pase con sanitizers.

## 3. Módulo de consola

- [ ] 3.1 Crear `source/suite.c` e `include/suite.h` con `Suite_IsRelaunch` y `Suite_Init` (Decisión 2), la copia y la restauración de `config.cfg` y `lastdir.txt` (Decisión 4) y las líneas `script` de `event=suite`, `scenario` y `error`. Cambiar `DebugLog_Init` para que reciba `relaunch` y no rote en ese caso, y agregar `DebugLog_Sync`, `DebugLog_LastState` y la llamada a `Suite_Observe` con la comprobación de hilo (Decisión 5 y Riesgos). Ordenar el arranque en `main.c` como en la Decisión 2. Listo cuando `scripts/build.sh` compile sin warnings y en la consola:
  - el escenario "Suite sin marcador" pase;
  - el escenario "Error en la consola" pase con `tests/console/dev-console-scripts-error.txt`, escrito en esta tarea;
  - una suite válida registre `event=suite` y `event=scenario` y cree `suite/` con la copia de `config.cfg`.
- [ ] 3.2 Inyección en `Utils_ReadControls`, `Touch_Update` y `UI_Debug_Update` (Decisión 3), con `Suite_Tick` y `Suite_Abort`, y `Music_SetSeed` (Decisión 8). Listo cuando `scripts/build.sh` compile y en la consola, con una suite de un solo escenario, pasen "Bajar dos filas", "Mantener abajo", "Toque inyectado", "El estado actual ya cumple", "Timeout" y "Abortar con un botón", comparando con las líneas de la tabla de la estrategia de pruebas.
- [ ] 3.3 Fin de escenario, relanzar y salir (Decisión 6), y el indicador (Decisión 7). Listo cuando `scripts/build.sh` compile y en la consola pasen "Suite con marcador", "Un ajuste no pasa al escenario siguiente" (MD5 de `config.cfg` antes y después desde la PC), "Una pista no sigue sonando", "Mismo orden con la misma semilla", "Suite completa", "Cerrar con PS a mitad de la suite", "Indicador visible" y "La suite queda en un solo archivo", con `tests/console/dev-console-scripts.txt` completa.

## 4. Suites, proceso y documentación

- [ ] 4.1 Escribir `tests/console/README.md` y `tests/console/dev-debug-log.txt` (Decisión 9), con la cabecera de fixtures. Listo cuando `dev-debug-log.txt` pase entera en la consola, y cada línea que verifica sus escenarios coincida con la del spec de `dev/debug-log`.
- [ ] 4.2 Agregar a `rules.design` en `openspec/config.yaml` la regla de la Decisión 9. Listo cuando `openspec validate add-console-scripts` pase y `openspec instructions design --change add-console-scripts --json` muestre la regla nueva.
- [ ] 4.3 Agregar al `README.md` la sección en inglés de la Decisión 10. Listo cuando explique `suite.txt`, el marcador, la salida automática, el aborto con un botón y dónde queda el resultado.

## 5. Verificación y versión

- [ ] 5.1 Memoria (estrategia de pruebas, paso 4): anotar el HEAP del overlay en Carpetas sin suite, con una suite en curso y después de abortarla, y el `heap_kb` de la primera línea `screen` de cada escenario de `dev-console-scripts.txt`. Anotar los resultados en esta tarea. Listo cuando el HEAP después de abortar vuelva al de sin suite, con una diferencia de no más de unos pocos KB, y los `heap_kb` de los escenarios coincidan. Si no, pausar y revisar con el usuario.
- [ ] 5.2 Correr las tres suites de `tests/console/` seguidas y los escenarios manuales que no cubrieron 3.x, y anotar fecha y resultado en esta tarea. Listo cuando todo pase, `make -C tests` pase y `scripts/build.sh` compile sin warnings.
- [ ] 5.3 Agregar las notas de versión de `proposal.md` a `[Unreleased]` en `CHANGELOG.md` y llevar `VITA_VERSION` en `CMakeLists.txt` de `"04.20"` a `"04.30"`. Listo cuando `scripts/build.sh` compile y la línea `start` del log diga `version=04.30`.
