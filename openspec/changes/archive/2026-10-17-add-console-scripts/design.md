## Context

La motivación está en `proposal.md`, y el comportamiento en `specs/dev/console-scripts/spec.md` y en el delta de `specs/dev/debug-log/spec.md`. Aquí solo va lo que da forma a la implementación.

- **Dónde se lee el input:**
  - `Utils_ReadControls` (`utils.c`): todas las pantallas, el diálogo de búsqueda y los escaneos de la biblioteca la llaman una vez por fotograma. Ahí está el único `sceCtrlPeekBufferPositive` de la app, aparte del overlay.
  - `UI_Debug_Update` (`ui_gpu.c:552`): el overlay lee el pad por su cuenta para L + R + SELECT y para guardar capturas.
  - `Touch_Update` (`touch.c`): el único `sceTouchPeek`. Pasa las coordenadas del panel a píxeles con `lerp` y registra `touch` en el log.
- **El log** (`debug_log.c`) ya guarda la última línea de estado de cada pantalla para descartar las repetidas, y todas sus líneas pasan por `DebugLog_Commit` o `DebugLog_CommitState`. Hoy todos los eventos nacen en el hilo principal.
- **El arranque** (`main.c`): `sceIoMkdir` → `DebugLog_Init` → `Config_Load` → `Config_GetLastDirectory` → … → bucle de pantallas, que empieza en Carpetas. La app no tiene salida propia: se cierra con PS.
- **Estado en la tarjeta que un escenario puede cambiar:** `config.cfg` (ajustes) y `lastdir.txt` (la carpeta en que abre Carpetas). Las playlists, `libraryroot.txt`, `library.txt` y `covers/` también, pero se tratan como fixtures (ver la proposal).
- **Aleatorio:** `Music_SeedOnce` (`menu_audioplayer.c`) llama a `srand(time())` la primera vez que hace falta, y `queue.c` usa `rand()` para mezclar.
- **Relanzar:** el VitaSDK declara `sceAppMgrLoadExec(const char *appPath, char * const argv[], const SceAppMgrExecOptParam *optParam)` en `psp2/appmgr.h`. RetroArch lo usa en la Vita para cambiar de eboot, pero no está probado en esta app ni con esta consola.

## Goals / Non-Goals

**Goals:**
- Que el parseo, el avance de los pasos fotograma a fotograma, la evaluación de un `wait` y el archivo de estado sean lógica pura, probada en PC con sanitizers.
- Que con el marcador ausente o sin `suite.txt`, el costo sea un `sceIoGetstat` al arrancar y la lectura de un booleano por fotograma.
- Que un escenario no herede nada de memoria del anterior: proceso nuevo cada vez.

**Non-Goals:**
- Un lenguaje con variables, condicionales o bucles. `xN` en `press` es la única repetición.
- Exigir orden entre los `wait` de un mismo grupo (ver Decisión 5).
- Mostrar el indicador durante los escaneos de la biblioteca y el diálogo de búsqueda, que dibujan su propio fotograma sin pasar por el overlay.

## Decisions

### 1. Dos módulos: `script.c` puro y `suite.c` de consola

- **`source/script.c` / `include/script.h`**, sin SCE:
  - `Script_Parse`: texto → `Script` (escenarios y pasos). En caso de error devuelve el número de línea y el motivo.
  - `ScriptRun`: la máquina de estados de un escenario. Cada fotograma recibe el tiempo en µs y devuelve un `ScriptFrame` (botones apretados, si hay dedo y dónde, si empezó un paso nuevo, si el escenario terminó y con qué resultado).
  - `ScriptRun_Observe`: recibe cada línea del log ya formateada y marca los `wait` pendientes que cumple.
  - `LogLine_Parse`: el inverso del formato del log (`<tiempo> <categoría> k=v ...`, con comillas y escapes). Vive en `log_line.c`, junto al formateo, y sirve para leer tanto las líneas que se registran como los valores de un `wait`.
  - `SuiteState_Format` / `SuiteState_Parse`: el archivo de estado (Decisión 4).
- **`source/suite.c` / `include/suite.h`**, con SCE: lee `suite.txt`, guarda y restaura la configuración, escribe el estado, relanza y sale. Además expone lo que necesitan los puntos de inyección.

*Por qué:* lo difícil de acertar (sintaxis, tiempos de `press` y `hold`, interpolación del `swipe`, qué línea cumple un `wait`, timeouts) se puede probar en PC fotograma a fotograma, con un reloj falso. Es el mismo patrón de `log_line.c` y `debug_log.c`.

### 2. Arranque y activación

`main.c`, después de `sceIoMkdir`:

```c
int relaunch = Suite_IsRelaunch();       // lee suite/state, sin efectos
DebugLog_Init(relaunch);                 // con relaunch: no rota, abre en append
Suite_Init(relaunch);                    // antes de Config_Load
Config_Load();
```

`Suite_Init`:
1. Si el log no está activo, termina. `suite.txt` no se toca.
2. Si existe `suite/state` y **no** es un relanzamiento, la sesión anterior se cerró a mitad de un escenario: registra `result=interrupted` para ese escenario y `event=end` con los totales, restaura la configuración, renombra `suite.txt` a `suite.done.txt`, borra `suite/` y termina. La app arranca normal.
3. Si no existe `suite.txt`, termina.
4. Lee `suite.txt` (máximo 64 KB) y lo parsea. Si tiene un error, registra `event=error`, lo renombra y termina.
5. Si es la primera vez (no hay estado), crea `suite/`, copia `config.cfg` y `lastdir.txt` ahí (o anota que no existían), registra `event=suite` y escribe el estado con `next=0`.
6. Restaura `config.cfg` y `lastdir.txt` desde `suite/`, escribe el estado con `relaunch=0` y `current=<n>`, registra `event=scenario` y prende `suite_running`.

Restaurar en el paso 6, antes de `Config_Load`, garantiza que el escenario arranca con la configuración original aunque el anterior la haya cambiado. Volver a restaurar al final es lo que deja la tarjeta limpia.

### 3. Inyección

- **`Utils_ReadControls`:** después de leer el pad físico, si `Suite_Running()`:
  - Si hay un botón físico nuevo (`pad.buttons & ~old_physical`), llama a `Suite_Abort()`.
  - Si no, llama a `Suite_Tick(now)`, que avanza el `ScriptRun` un fotograma, y reemplaza `pad.buttons` por los botones del `ScriptFrame`.

  Todo lo demás (log `input`, `ScreenOff_Filter`, `KeyRepeat`) sigue igual, así que el log registra el input del guion como si fuera físico y la autorrepetición funciona con `hold`. Como `Utils_ReadControls` también corre durante los escaneos y el diálogo de búsqueda, el guion avanza ahí también. Es un solo tick por fotograma: ningún otro lugar lo avanza.
- **`Touch_Update`:** si `Suite_Running()`:
  - Si el panel físico informa un dedo, llama a `Suite_Abort()`.
  - Si no, usa el dedo del `ScriptFrame` del fotograma ya en píxeles de pantalla, sin pasar por `lerp`. El resto de la función (flancos, log) sigue igual, así que `tap 480 272` se registra con `x=480 y=272`.
- **`UI_Debug_Update`:** si `Suite_Running()`, usa `Utils_HeldButtons()` (que ya devuelve el pad del fotograma, inyectado) en vez de leer el pad. Así un guion puede abrir el overlay y un botón físico no lo abre por su cuenta.

**Tiempos de los pasos:**

| Paso | Qué entrega |
|---|---|
| `press` | un fotograma con los botones y uno sin ellos, por cada repetición. Así cada repetición es un flanco y la autorrepetición, que necesita cientos de ms, no se dispara. |
| `hold` | los botones durante la duración, medida con el reloj, y después un fotograma sin ellos. |
| `tap` | el dedo en (x, y) durante 100 ms y después un fotograma sin dedo. Es lo que dura un toque real, y el reconocedor de gestos lo ve como un toque y no como una pulsación larga. |
| `swipe` | el dedo interpolado en línea recta según el tiempo transcurrido, y al final un fotograma sin dedo. |

### 4. Archivos en la tarjeta

```
ux0:data/ElevenMPV/
  debug                 marcador del log (ya existe)
  suite.txt             la suite (la copia quien despliega)
  suite.done.txt        la suite ya ejecutada, abortada o con error
  suite/                existe solo mientras corre una suite
    state               next, current, relaunch, pass, fail
    config.cfg          copia del original (si existía)
    lastdir.txt         copia del original (si existía)
    noconfig, nolastdir marcadores de "no existía": restaurar = borrar
```

`state` es texto `clave=valor`, una por línea, y se escribe completo cada vez: a un archivo temporal y luego `sceIoRename`. Así un cierre a mitad de la escritura deja el estado anterior entero.

### 5. Qué cumple un `wait`

- `debug_log.c` llama a `Suite_Observe(line)` en `DebugLog_Commit` y en `DebugLog_CommitState`, aunque la línea de estado se descarte por repetida, y solo si `Suite_Running()`. La llamada pasa la línea ya formateada a `ScriptRun_Observe`.
- **Lookahead:** cuando empieza un paso de acción (`press`, `hold`, `tap`, `swipe`), `ScriptRun` arma la lista de los `wait` que siguen hasta la próxima acción, saltando `sleep` y `seed`. Cada línea observada desde ese momento marca los `wait` de esa lista que cumple. Cuando le toca a un `wait`, si ya está marcado se cumple enseguida. Así una línea que llega en el mismo fotograma que el `press`, antes de que el `wait` empiece, no se pierde, y no hace falta guardar líneas en memoria.
- **Estado actual:** al empezar un `wait` de una categoría de estado, también se evalúa contra la última línea de ese canal, que `debug_log.c` ya guarda (`DebugLog_LastState`).
- Los `wait` de un mismo grupo no exigen orden entre sí: `wait library sel=1` y `wait library sel=2` después de `press DOWN x2` pasan aunque el orden real fuera otro. Para exigir un orden, el guion pone una acción entre los dos.
- **Timeout:** se mide desde que el `wait` empieza. Si vence, el escenario termina con `fail` y el motivo `reason="wait timeout" line=<n> last="<última línea de esa categoría>"`. Para eso, `ScriptRun` guarda la última línea observada de la categoría de cada `wait` pendiente (un `LogLine` de 512 B).

### 6. Fin de escenario, relanzar y salir

Al terminar un escenario, con `pass` o `fail`:
1. Registra `event=result`.
2. Restaura `config.cfg` y `lastdir.txt`.
3. Si quedan escenarios: escribe el estado con `next+1` y `relaunch=1`, llama a `DebugLog_Sync()` (nuevo: vacía el anillo a la tarjeta desde el hilo principal y espera a que termine) y llama a `sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL)`. Si vuelve, falló: registra `event=abort reason="relaunch <código>"`, termina la suite como en un aborto y la app sigue con el input físico.
4. Si era el último: registra `event=end`, renombra `suite.txt`, borra `suite/`, muestra el resumen 3 segundos (o hasta que se apriete un botón físico), llama a `DebugLog_Sync()` y a `sceKernelExitProcess(0)`.

`Suite_Abort()` registra `event=abort reason=user`, restaura, renombra, borra `suite/` y apaga `suite_running`. No relanza: la app sigue en la pantalla en que estaba y el usuario la usa normalmente.

*Alternativa descartada:* un comando `reset` que lleve la app a Carpetas, cierre menús y pare la música, sin relanzar. Cada pantalla nueva tendría que saber cómo volver a cero, y un bug que deja estado sucio contaminaría el escenario siguiente. Queda como plan B si el spike de `LoadExec` falla (tarea 1.1).

### 7. Indicador

`UI_Debug_Draw`, que las cuatro pantallas ya llaman al final del fotograma, dibuja con `Suite_Running()` una franja arriba con `<i>/<total>  <nombre>  ·  cualquier botón detiene`, y al terminar la suite, el resumen. Va encima del overlay, con la fuente mono que el overlay ya usa. No se dibuja en los fotogramas de los escaneos ni del diálogo de búsqueda (Non-Goals).

### 8. Semilla

`seed <n>` llama a `Music_SetSeed(n)` (nuevo en `menu_audioplayer.c`), que hace `srand(n)` y marca `Music_SeedOnce` como hecha, para que no la pise con `time()`. Como cada escenario es un proceso nuevo, la semilla no pasa de uno a otro.

### 9. Suites versionadas

- `tests/console/README.md`: el formato (resumen del spec), cómo correr una suite y la convención de fixtures. Cada suite empieza con un comentario que dice qué necesita en la tarjeta (por ejemplo `# fixtures: biblioteca de Mis cosas escaneada, ux0:/pruebas-eleven/formatos/`).
- Suites iniciales:
  - `tests/console/dev-console-scripts.txt`: los escenarios automatizables de este cambio.
  - `tests/console/dev-console-scripts-error.txt`: el guion con error de la línea 3.
  - `tests/console/dev-debug-log.txt`: "Recorrido corto por la Biblioteca", "Toque en la pantalla", "Pista que no abre" y "Fin de pista y avance de la cola", como prueba de que las suites cubren escenarios ya existentes.
- Una regla nueva en `rules.design` de `openspec/config.yaml`: si el cambio tiene escenarios de consola, el design indica cuáles se automatizan en una suite de `tests/console/` (y en cuál) y cuáles quedan manuales, con el motivo (imagen, fluidez, audio, o algo que el guion no puede hacer).

### 10. README

Una sección corta en inglés, debajo de la del log: qué es `suite.txt`, que necesita el marcador, que la app sale sola al terminar, que cualquier botón la detiene, y que el resultado queda en `debug.log`.

## Recursos

| Recurso | Lo toma | Lo libera |
|---|---|---|
| Texto de `suite.txt` (`malloc`, ≤ 64 KB) | `Suite_Init`, paso 4 | `Suite_Init` apenas termina el parseo, con éxito o con error |
| `Script` parseado (`malloc`, un bloque) | `Script_Parse` | `Suite_Abort`, el fin de la suite, o `Suite_Init` si el parseo falla; con `LoadExec` o `sceKernelExitProcess` el sistema recupera todo |
| Descriptores de `suite/state`, copias de config | cada lectura o escritura | la misma función, siempre, también si falla |
| Directorio `suite/` | `Suite_Init`, paso 5 | el fin de la suite, `Suite_Abort`, o el arranque que detecta un `interrupted` |

`Script` es un solo bloque: un arreglo de pasos de tamaño fijo (máximo 1024) y un pool de texto para nombres, comandos y condiciones. Sin el marcador o sin `suite.txt`, no se reserva nada.

## Estrategia de pruebas

1. **Tests en PC: `tests/test_script.c`**, que se suma a `BIN` en `tests/Makefile` y corre con ASan y UBSan:
   - parseo: cada comando, comentarios, líneas vacías, comillas y escapes en `wait`, duraciones en `ms` y `s`, botones combinados, `xN`;
   - errores con su línea y su motivo: comando y botón desconocidos, argumentos de más y de menos, coordenada fuera de rango, duración inválida, paso antes del primer `scenario`, archivo sin escenarios, más de 1024 pasos;
   - runner con reloj falso: `press DOWN x2` da abajo, nada, abajo, nada; `hold DOWN 2s` mantiene el botón hasta los 2 s según el reloj; `tap` dura 100 ms; `swipe` interpola los extremos y el punto medio;
   - `wait`: campos de más, valor distinto, valor con comillas, línea que llega antes de que el `wait` empiece (lookahead), estado actual, timeout con `line` y `last`, y un `wait` sin acción previa;
   - `LogLine_Parse`: ida y vuelta con `LogLine_Str` para valores con espacios, comillas, barras y saltos de línea, y una línea recortada con `cut=1`;
   - `SuiteState`: ida y vuelta, y un archivo truncado o con basura, que se trata como ausente;
   - camino de error: un guion de 64 KB de basura y uno con el máximo de pasos, sin escribir fuera del bloque.
2. **Compilación:** `scripts/build.sh` sin warnings con `-Werror`.
3. **Consola.** Las suites de `tests/console/` se despliegan con la Vita en modo USB, el usuario sale del modo USB, abre la app y vuelve al modo USB cuando la app se cierra, y se lee `debug.log`:

   | Escenario del spec | Cómo | Líneas que lo verifican |
   |---|---|---|
   | Suite sin marcador | manual (es la ausencia del log) | no existe `debug.log`; `suite.txt` intacto |
   | Suite con marcador | `dev-console-scripts.txt` | `script event=suite scenarios=N`, `script event=result ... result=pass` |
   | Error en la consola | `dev-console-scripts-error.txt` | `script event=error line=3`; ninguna `script event=step`; existe `suite.done.txt` |
   | Bajar dos filas | `dev-console-scripts.txt` | `input down=DOWN` / `up=DOWN` ×2, `library sel=1`, `library sel=2`, `result=pass` |
   | Mantener abajo | `dev-console-scripts.txt` | una `input down=DOWN`, una `input up=DOWN` ~2 s después, varias `library` con `sel` creciente |
   | Toque inyectado | `dev-console-scripts.txt` | `touch event=down x=480 y=272`, `touch event=up x=480 y=272` |
   | El estado actual ya cumple | `dev-console-scripts.txt` | dos `script event=step` de `wait` seguidos sin `input` entre ellos, `result=pass` |
   | Timeout | `dev-console-scripts.txt` (escenario que falla a propósito) | `script event=result result=fail` con `line=` y `last="library ..."` |
   | Un ajuste no pasa al siguiente | `dev-console-scripts.txt` | la `settings` del segundo escenario; MD5 de `config.cfg` antes y después, desde la PC |
   | Una pista no sigue sonando | `dev-console-scripts.txt` | `log event=start` entre escenarios; ninguna `track` antes del primer `press` del segundo |
   | Abortar con un botón | manual | `script event=abort reason=user`, nada `script` después; existe `suite.done.txt` |
   | Cerrar con PS a mitad | manual | `script event=result result=interrupted`; existe `suite.done.txt`; no existe `suite/` |
   | Suite completa | `dev-console-scripts.txt` | `event=result` por escenario; última `script` con `event=end pass=… fail=1` (el escenario del timeout) |
   | Indicador visible | manual (imagen) | — |
   | Mismo orden con la misma semilla | `dev-console-scripts.txt` | las `track event=open` de los dos escenarios coinciden |
   | La suite queda en un solo archivo (delta de `dev/debug-log`) | `dev-console-scripts.txt` | un `log event=start` por escenario en el mismo `debug.log`; `debug.prev.log` es la sesión anterior |

   Además, `dev-debug-log.txt` tiene que pasar entera: confirma que las suites reproducen escenarios que hoy se hacen a mano.
4. **Memoria**, según la regla del proyecto: con el overlay, anotar el HEAP en Carpetas sin suite, con una suite en curso y después de abortarla. Lo de la suite se libera al abortar, y el HEAP tiene que volver al valor sin suite, con una diferencia de no más de unos pocos KB. Entre escenarios no hay nada que medir, porque cada uno es un proceso nuevo, pero el `heap_kb` de la línea `screen` de cada escenario tiene que ser el mismo.

## Cambios en CI

Ninguno. `test_script` entra por `BIN`, y los archivos nuevos por `add_executable` en `CMakeLists.txt`.

## Código de terceros

Ninguno.

## Risks / Trade-offs

- **[`sceAppMgrLoadExec` no funciona en esta app o en esta consola]** → La tarea 1.1 es un spike antes de cualquier otra cosa. Si falla, se pausa y se revisa con el usuario el plan B (comando `reset`, Decisión 6), que cambia el requirement "Cada escenario empieza desde el mismo estado".
- **[Relanzar con audio sonando o recursos de GPU tomados]** El proceso se reemplaza entero, y el sistema recupera todo como con el botón PS. → El spike relanza con una pista sonando y comprueba que el escenario siguiente arranca limpio.
- **[Una suite que nunca termina]** Por ejemplo, un `sleep` largo o una pantalla que se cuelga. → Cada `wait` tiene timeout, y el usuario corta con cualquier botón. Si la app se cuelga del todo, se cierra con PS y el próximo arranque registra `interrupted`.
- **[Un `wait` se cumple con una línea vieja]** → Solo cuentan las líneas desde la última acción y el estado actual. Un guion que espera algo que ya pasó antes de la acción anterior falla con timeout, y el motivo muestra la última línea.
- **[Un evento futuro nace en otro hilo]** `Suite_Observe` no es thread-safe. → Solo se llama desde el hilo principal. `debug_log.c` lo comprueba con el id del hilo principal, guardado en `DebugLog_Init`, y no observa líneas de otros hilos. Si un cambio futuro registra desde el audio, tendrá que revisar esto.
- **[El usuario toca la consola sin querer]** La suite se aborta. → Es lo deseado: la consola es del usuario. Se vuelve a correr.
- **[La configuración restaurada pisa un cambio real]** Si el usuario aborta y antes un escenario había cambiado un ajuste, el ajuste vuelve al original. → Es lo esperado: la suite no deja rastros.
- **[Fixtures fuera de lo que se restaura]** Un escenario que guarda una playlist la deja en la tarjeta. → Las suites nombran lo que crean con el prefijo `suite-` y lo documentan en su cabecera, para que el despliegue lo limpie.

## Migration Plan

No hay datos que migrar. Sin `suite.txt` no cambia nada. Para volver atrás basta con borrar `suite.txt` y `suite/`.
